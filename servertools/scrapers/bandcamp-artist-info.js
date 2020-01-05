var bandcamp = require('bandcamp-scraper');

// check search pattern
if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[0] + " " + process.argv[1] + " ARTIST-URL");
    process.exit()
}
var artistUrl = process.argv[2].replace("https://", "http://");

var albums = [];
var left = 0;

function sanitizeStr(str) {
    return str.split("\n")[0];
}

function done() {
    left -= 1;
    if (left == 0) {
        console.log("[");
        for (var i = 0; i < albums.length; ++i) {
            var str = JSON.stringify(albums[i], null, 2);
            if (i < albums.length - 1)
                str = str + ",";
            console.log(str);
        }
        console.log("]");
    }
}

function handler(error, info) {
    if (error)
        return;

    var albums = [];
    info.forEach(function(albumInfo) {
        if (albumInfo.url.includes("/album/")) {
            // badly fix album name for single-album artists
            if (albumInfo.title == "") {
                var parts = albumInfo.url.split("/");
                albumInfo.title = parts[parts.length - 1];
            }
            
            // ignore double albums
            var hasSameUrl = function(album) { return album.url == albumInfo.url; };
            if (albums.some(hasSameUrl))
                return;
            
            albums.push({
                type: "album",
                name: sanitizeStr(albumInfo.title),
                icon: albumInfo.imageUrl,
                url: albumInfo.url,
            });
        }
    });

    console.log(JSON.stringify(albums, null, 2));

//    info.forEach(function(albumUrl) {
//        if (albumUrl.includes("/album/")) {
//            left += 1;
//            
//            var albumHandler = function(error, info) {
//                if (!error)
//                    albums.push({
//                        name: info.title,
//                        url: albumUrl,
//                        icon: info.imageUrl,
//                        tracks: info.tracks
//                    });
//                
//                done();
//            };
//            
//            bandcamp.getAlbumInfo(albumUrl, albumHandler);
//        }
//    });
}

bandcamp.getAlbumUrls(artistUrl, handler);
