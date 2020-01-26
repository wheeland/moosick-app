var youtube = require('scrape-youtube');
var spawn = require('child_process');
var fs = require('fs');

if (process.argv.length < 3) {
    console.log("Usage: " + process.argv[1] + " URL");
    process.exit()
}
let url = process.argv[2];

let ytId = url.split("=")[1];
let filename = "tmp_ytgi_" + ytId + ".out";
let command = 'sh -c \'youtube-dl -j "' + url + '" > ' + filename + '\'';

spawn.exec(command, function(error, stdout, stderr) {
    let resultJson = fs.readFileSync(filename, 'utf8');
    fs.unlinkSync(filename);
    let result = JSON.parse(resultJson);
    let output = {
        title: result.title ? result.title : "",
        duration: result.duration ? result.duration : 0,
        chapters: result.chapters ? result.chapters : [],
    }
    console.log(JSON.stringify(output, null, 2));
});

