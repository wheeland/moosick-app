#pragma once

#include "library.hpp"
#include "jsonconv.hpp"

namespace MoosickMessage {

enum class Type : quint32
{
    Error,
    Ping,
    Pong,

    /** Retrieve current state of the library */
    LibraryRequest,
    LibraryResponse,

    /** Retrieve base URL for media files */
    MediaUrlRequest,
    MediaUrlResponse,

    /** Retrieve library ID */
    IdRequest,
    IdResponse,

    /** Apply a set of changes to the library, return applied changes */
    ChangesRequest,
    ChangesResponse,

    /** Upload a song and return its ID */
    UploadSongRequest,
    UploadSongRequestInternal,
    UploadSongResponse,

    /** Request a list of changes made since revision N */
    ChangeListRequest,
    ChangeListResponse,

    /** Download from YT/BC, return a list of running download IDs */
    DownloadRequest,
    DownloadResponse,
    DownloadQuery,
    DownloadQueryResponse,

    /** Request youtube video details to be queried by youtube-dl */
    YoutubeUrlQuery,
    YoutubeUrlResponse,
};

QString typeString(Type messageType);

enum class DownloadRequestType
{
    BandcampAlbum,
    YoutubeVideo,
    YoutubePlaylist,
};

} // namespace MoosickMessage

// needs to be declared outside of namespace
ENJSON_DECLARE_ALIAS(MoosickMessage::DownloadRequestType, quint32)

namespace MoosickMessage {

struct MessageBase
{
    virtual ~MessageBase() {}
    virtual QString getMessageTypeString() const { return typeString(getMessageType()); }
    virtual Type getMessageType() const = 0;
    virtual QJsonValue enjson() const = 0;
};

#define DEFINE_MESSAGE_TYPE(TYPE) \
    ENJSON_OBJECT(TYPE) \
    static constexpr Type MESSAGE_TYPE = Type::TYPE; \
    struct MESSAGE_MARKER {}; \
    Type getMessageType() const override { return Type::TYPE; } \
    QJsonValue enjson() const override { return ::enjson(*this); } \

struct Error : public MessageBase
{
    Error() = default;
    Error(const QString &msg) { errorMessage = msg; }
    DEFINE_MESSAGE_TYPE(Error)
    ENJSON_MEMBER(QString, errorMessage);
};

struct Ping : public MessageBase
{
    DEFINE_MESSAGE_TYPE(Ping)
};

struct Pong : public MessageBase
{
    DEFINE_MESSAGE_TYPE(Pong)
};

struct LibraryRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(LibraryRequest)
};

struct LibraryResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(LibraryResponse)
    ENJSON_MEMBER(quint32, version);
    ENJSON_MEMBER(QJsonObject, libraryJson);
};

struct MediaUrlRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(MediaUrlRequest)
};

struct MediaUrlResponse : public MessageBase
{
    MediaUrlResponse() = default;
    MediaUrlResponse(const QString &u) { url = u; }
    DEFINE_MESSAGE_TYPE(MediaUrlResponse)
    ENJSON_MEMBER(QString, url);
};

struct IdRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(IdRequest)
};

struct IdResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(IdResponse)
    ENJSON_MEMBER(QString, id);
};

struct ChangesRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(ChangesRequest)
    ENJSON_MEMBER(QVector<Moosick::LibraryChangeRequest>, changes);
};

struct ChangesResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(ChangesResponse)
    ENJSON_MEMBER(QVector<Moosick::CommittedLibraryChange>, changes);
};

struct UploadSongRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(UploadSongRequest)
    ENJSON_MEMBER(QString, artistName);
    ENJSON_MEMBER(QString, albumName);
    ENJSON_MEMBER(QString, title);
    ENJSON_MEMBER(int, position);
    ENJSON_MEMBER(int, duration);
    ENJSON_MEMBER(int, fileSize);
    ENJSON_MEMBER(QString, fileEnding);
};

struct UploadSongRequestInternal : public MessageBase
{
    DEFINE_MESSAGE_TYPE(UploadSongRequestInternal)
    ENJSON_MEMBER(QString, artistName);
    ENJSON_MEMBER(QString, albumName);
    ENJSON_MEMBER(QString, title);
    ENJSON_MEMBER(int, position);
    ENJSON_MEMBER(int, duration);
    ENJSON_MEMBER(QString, filePath);
    ENJSON_MEMBER(QString, fileEnding);
};

struct UploadSongResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(UploadSongResponse)
    ENJSON_MEMBER(quint32, songId);
};

struct ChangeListRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(ChangeListRequest)
    ENJSON_MEMBER(quint32, revision);
};

struct ChangeListResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(ChangeListResponse)
    ENJSON_MEMBER(QVector<Moosick::CommittedLibraryChange>, changes);
};

struct DownloadRequest : public MessageBase
{
    DEFINE_MESSAGE_TYPE(DownloadRequest)
    ENJSON_MEMBER(DownloadRequestType, requestType);
    ENJSON_MEMBER(QString, url);
    ENJSON_MEMBER(quint32, artistId);
    ENJSON_MEMBER(QString, artistName);
    ENJSON_MEMBER(QString, albumName);
    ENJSON_MEMBER(quint32, currentRevision);
};

struct DownloadResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(DownloadResponse)
    ENJSON_MEMBER(quint32, downloadId);
};

struct DownloadQuery : public MessageBase
{
    DEFINE_MESSAGE_TYPE(DownloadQuery)
};

struct DownloadQueryResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(DownloadQueryResponse)
    using ActiveDownload = QPair<quint32, DownloadRequest>;
    ENJSON_MEMBER(QVector<ActiveDownload>, activeRequests);
};

struct YoutubeUrlQuery : public MessageBase
{
    DEFINE_MESSAGE_TYPE(YoutubeUrlQuery)
    ENJSON_MEMBER(QString, videoId)
};

struct YoutubeUrlResponse : public MessageBase
{
    DEFINE_MESSAGE_TYPE(YoutubeUrlResponse)
    ENJSON_MEMBER(QString, title)
    ENJSON_MEMBER(int, duration)
    ENJSON_MEMBER(QJsonArray, chapters)
    ENJSON_MEMBER(QString, url)
};

#undef DEFINE_MESSAGE_TYPE

class Message
{
public:
    Message() = default;
    Message(MessageBase *msg) : m_msg(msg) {}
    Message(const Message &) = delete;
    Message(Message &&rhs) { *this = std::move(rhs); }
    Message& operator=(Message &&rhs) { m_msg.reset(rhs.m_msg.take()); return *this; }

    template <class MessageClass>
    Message(const MessageClass &msg, typename MessageClass::MESSAGE_MARKER = typename MessageClass::MESSAGE_MARKER())
        : m_msg(new MessageClass(msg))
    {
    }

    Type getType() const { return m_msg ? m_msg->getMessageType() : Type::Error; }
    QString getTypeString() const { return m_msg ? m_msg->getMessageTypeString() : QString("null"); }

    template <class T> T *as()
    {
        return (m_msg && m_msg->getMessageType() == T::MESSAGE_TYPE) ? static_cast<T*>(m_msg.data()) : nullptr;
    }
    template <class T> const T *as() const
    {
        return (m_msg && m_msg->getMessageType() == T::MESSAGE_TYPE) ? static_cast<const T*>(m_msg.data()) : nullptr;
    }

    MessageBase *operator->() { return m_msg.data(); }
    const MessageBase *operator->() const { return m_msg.data(); }

    QByteArray toJson()  const;
    static Result<Message, EnjsonError> fromJson(const QByteArray &message);

    template <class T>
    static Result<T, EnjsonError> fromJsonAs(const QByteArray &message);

private:
    QScopedPointer<MessageBase> m_msg;
};

QByteArray messageToJson(const MessageBase &message);

template <class T>
Result<T, EnjsonError> Message::fromJsonAs(const QByteArray &message)
{
    Result<T, EnjsonError> ret;

    Result<Message, EnjsonError> error = Message::fromJson(message);
    if (error.hasError()) {
        ret.setError(error.takeError());
    }
    else {
        Message msg = error.takeValue();
        if (T *val = msg.as<T>()) {
            ret.setValue(T(std::move(*val)));
        }
        else {
            ret.setError(EnjsonError::buildCustomError(QString("Wrong message type: ") + msg->getMessageTypeString()));
        }
    }

    return ret;
}

} //namespace MoosickMessage
