var bandcamp = require('bandcamp-scraper');
var youtube = require('scrape-youtube');

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

function youtubeResults(searchResults) {
    searchResults.forEach(function(result) {
        if (result.type == "video") {
            results.push({
                type: "video",
                url: result.link,
                name: sanitizeStr(result.title),
                icon: result.thumbnail
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
    
    done();
}

bandcamp.search({ query: pattern, page: 1 }, bandcampSearchResult);
//bandcamp.search({ query: pattern, page: 2 }, bandcampSearchResult);

youtube(pattern, { limit : 10, type : "any" }).then(youtubeResults, function(err) {});
