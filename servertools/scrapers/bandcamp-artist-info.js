var bandcamp = require('bandcamp-scraper');

// check search pattern
if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[0] + " " + process.argv[1] + " ARTIST-URL");
    process.exit()
}
var artistUrl = process.argv[2].replace("https://", "http://");

var albums = [];

function albumHandler(error, info) {
    if (error)
        return;

    console.log({
        name: info.title,
        icon: info.imageUrl,
        tracks: info.tracks
    });
}

function handler(error, info) {
    if (error)
        return;

    info.forEach(function(url) {
        if (url.includes("/album/"))
            bandcamp.getAlbumInfo(url, albumHandler);
    });
}

bandcamp.getAlbumUrls(artistUrl, handler);
