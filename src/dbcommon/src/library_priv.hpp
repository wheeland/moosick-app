#include "library.hpp"

namespace Moosick {

struct Library::Song
{
    QString name;
    AlbumId album;
    quint32 fileEnding = 0;
    quint32 position = 0;
    quint32 secs = 0;
    TagIdList tags;
};

struct Library::Album
{
    QString name;
    ArtistId artist;
    SongIdList songs;
    TagIdList tags;
};

struct Library::Artist
{
    QString name;
    AlbumIdList albums;
    TagIdList tags;
};

struct Library::Tag
{
    QString name;
    TagId parent;
    TagIdList children;
    SongIdList songs;
    AlbumIdList albums;
    ArtistIdList artists;
};

} // namespace Moosick
