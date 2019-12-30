var bandcamp = require('bandcamp-scraper');
var youtube = require('scrape-youtube');

// check search pattern
if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[0] + " " + process.argv[1] + " SEARCH PATTERN");
    process.exit()
}
var pattern = process.argv[2];

function bandcampSearchResult(error, searchResults) {
    searchResults.forEach(function(result) {
        if (result.type === "album") {
            console.log(JSON.stringify({
                type: "album",
                url: result.url,
                name: result.name,
                icon: result.imageUrl
            }, null, 2) + ",");
        }
        else if (result.type === "artist") {
            console.log(JSON.stringify({
                type: "artist",
                url: result.url,
                name: result.name,
                icon: result.imageUrl
            }, null, 2) + ",");
        }
    });
};

function youtubeResults(results) {
    results.forEach(function(result) {
        if (result.type == "video") {
            console.log(JSON.stringify({
                type: "video",
                url: result.link,
                name: result.title,
                icon: result.thumbnail
            }, null, 2) + ",");
        }
        else if (result.type == "playlist") {
            console.log(JSON.stringify({
                type: "playlist",
                url: result.link,
                name: result.title,
                icon: result.thumbnail,
                count: result.video_count
            }, null, 2) + ",");
        }
    });
}

bandcamp.search({ query: pattern, page: 1 }, bandcampSearchResult);
//bandcamp.search({ query: pattern, page: 2 }, bandcampSearchResult);

youtube(pattern, { limit : 10, type : "any" }).then(youtubeResults, function(err) {});
