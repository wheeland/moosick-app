var bandcamp = require('bandcamp-scraper');

// check search pattern
if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[0] + " " + process.argv[1] + " ALBUM-URL");
    process.exit()
}
var albumUrl = process.argv[2].replace("https://", "http://");

function sanitizeStr(str) {
    return str.split("\n")[0];
}

function handler(error, info) {
    if (error)
        return;

    console.log(JSON.stringify({
        name: sanitizeStr(info.title),
        url: albumUrl,
        icon: info.imageUrl,
        tracks: info.tracks
    }, null, 2));
}

bandcamp.getAlbumInfo(albumUrl, handler);
