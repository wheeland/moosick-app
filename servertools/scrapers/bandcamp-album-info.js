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

function getFileUrl(fileinfo) {
    for (f in fileinfo) {
        if (fileinfo[f].startsWith("http"))
            return fileinfo[f];
    }
    return undefined;
}

function handler(error, info) {
    if (error)
        return;

    var tracks = [];
    for (var i = 0; i < info.tracks.length; ++i) {
        if (info.raw.trackinfo[i].streaming === 1) {
            var url = getFileUrl(info.raw.trackinfo[i].file);
            if (url) {
                tracks.push({
                    name: info.tracks[i].name,
                    url: url,
                    duration: info.tracks[i].duration,
                });
            }
        }
    }

    console.log(JSON.stringify({
        name: sanitizeStr(info.title),
        url: albumUrl,
        icon: info.imageUrl,
        tracks: tracks,
    }, null, 2));
}

bandcamp.getAlbumInfo(albumUrl, handler);
