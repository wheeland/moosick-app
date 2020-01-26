var bandcamp = require('bandcamp-scraper');
var youtube = require('scrape-youtube');
var spawn = require('child_process');
var fs = require('fs');

// check search pattern
if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[0] + " " + process.argv[1] + " SEARCH PATTERN");
    process.exit()
}
var pattern = process.argv[2];

var results = [];
var left = 2;

function done() {
    left -= 1;
    
    if (left === 0) {
        for (var i = 0; i < results.length; ++i) {
            var str = JSON.stringify(results[i], null, 2);
            if (i < results.length - 1)
                str = str + ","
            console.log(str);
        };
        console.log("]");
        process.exit();
    }
}

console.log("[");

function sanitizeStr(str) {
    return str.split("\n")[0];
}

function bandcampSearchResult(error, searchResults) {
    searchResults.forEach(function(result) {
        if (result.type === "album") {
            results.push({
                type: "album",
                artist: result.artist,
                url: result.url,
                name: sanitizeStr(result.name),
                icon: result.imageUrl
            });
        }
        else if (result.type === "artist") {
            results.push({
                type: "artist",
                url: result.url,
                name: sanitizeStr(result.name),
                icon: result.imageUrl
            });
        }
    });
    
    done();
};

function findBestFormat(formats) {
    if (formats.length == 0)
        return "";

    var bestAudioSize = 0;
    var bestUrl = "";
    for (var i = 0; i < formats.length; ++i) {
        // we ain't wanna deal with DASH
        if (formats[i].format.toLowerCase().includes("dash"))
            continue;
        // set fallback
        if (bestUrl === "")
            bestUrl = formats[i].url;
        // check if best audio format
        if (formats[i].format.includes("audio only") && formats[i].filesize && formats[i].filesize > bestAudioSize)
            bestUrl = formats[i].url;
    }

    return bestUrl;
}

function youtubeResults(searchResults) {
    var videos = [];
    
    searchResults.forEach(function(result) {
        if (result.type == "video") {
            videos.push({
                type: "video",
                url: result.link,
                name: sanitizeStr(result.title),
                icon: result.thumbnail,
                duration: result.duration,
            });
        }
        else if (result.type == "playlist") {
            results.push({
                type: "playlist",
                url: result.link,
                name: sanitizeStr(result.title),
                icon: result.thumbnail,
                count: result.video_count
            });
        }
    });
    
    for (var i = 0; i < videos.length; ++i) {
        let video = videos[i];
        let ytId = video.url.split("=")[1];
        let filename = "tmp_ytdl_" + ytId + ".out";
        let command = 'sh -c \'youtube-dl -j "' + videos[i].url + '" > ' + filename + '\'';
        
        spawn.exec(command, function(error, stdout, stderr) {
            let resultJson = fs.readFileSync(filename, 'utf8');
            fs.unlinkSync(filename);
            let result = JSON.parse(resultJson);
            video.audioUrl = findBestFormat(result.formats);
            results.push(video);
            done();
        });
    }
    left += videos.length;
    
    done();
}

bandcamp.search({ query: pattern, page: 1 }, bandcampSearchResult);
youtube(pattern, { limit : 5, type : "any" }).then(youtubeResults, function(err) {});

//bandcamp.search({ query: pattern, page: 2 }, bandcampSearchResult);

