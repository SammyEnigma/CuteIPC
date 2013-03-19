// Local
#include "CuteIPCMarshaller_p.h"
#include "CuteIPCMessage_p.h"

// Qt
#include <QDataStream>
#include <QPair>
#include <QMetaType>
#include <QtGui/QImage>
#include <QBuffer>
#include <QDebug>


QByteArray CuteIPCMarshaller::marshallMessage(const CuteIPCMessage& message)
{
  QByteArray result;
  QDataStream stream(&result, QIODevice::WriteOnly);

  stream << message.messageType();
  stream << message.method();
  stream << message.returnType();
  stream << message.arguments().size();

  bool successfullyMarshalled;
  foreach (const QGenericArgument& arg, message.arguments())
  {
    successfullyMarshalled = marshallArgumentToStream(arg, stream);
    if (!successfullyMarshalled)
      return QByteArray();
  }

  return result;
}


CuteIPCMessage CuteIPCMarshaller::demarshallMessage(QByteArray& call)
{
  QDataStream stream(&call, QIODevice::ReadOnly);

  // Call type
  CuteIPCMessage::MessageType type;
  int buffer;
  stream >> buffer;
  type = CuteIPCMessage::MessageType(buffer);

  // Method
  QString method;
  stream >> method;

  QString returnType;
  stream >> returnType;

  // Arguments
  int argc = 0;
  stream >> argc;
  Q_ASSERT(argc <= 10);

  CuteIPCMessage::Arguments args;
  for (int i = 0; i < argc; ++i)
  {
    bool ok;
    QGenericArgument argument = demarshallArgumentFromStream(ok, stream);
    if (!ok)
    {
      qWarning() << "CuteIPC:" << "Failed to deserialize argument" << i;
      break;
    }
    args.append(argument);
  }

  return CuteIPCMessage(type, method, args, returnType);
}


CuteIPCMessage::MessageType CuteIPCMarshaller::demarshallMessageType(QByteArray& message)
{
  QDataStream stream(&message, QIODevice::ReadOnly);
  // Call type
  int buffer;
  stream >> buffer;
  return CuteIPCMessage::MessageType(buffer);
}


CuteIPCMessage CuteIPCMarshaller::demarshallResponse(QByteArray& call, QGenericReturnArgument arg)
{
  QDataStream stream(&call, QIODevice::ReadOnly);

  CuteIPCMessage::MessageType type;
  int buffer;
  stream >> buffer;
  type = CuteIPCMessage::MessageType(buffer);

  QString method;
  stream >> method;

  QString returnType;
  stream >> returnType;

  CuteIPCMessage::Arguments args; //construct message with empty arguments

  int argc;
  stream >> argc;
  if (argc != 0) // load returned value to the arg (not to the message)
  {
    QString typeName;
    stream >> typeName;

    // Check type
    int type = QMetaType::type(typeName.toLatin1());
    if (type == 0)
      qWarning() << "CuteIPC:" << "Unsupported type of argument " << ":" << typeName;

    if (arg.name() && arg.data())
    {
      if (type != QMetaType::type(arg.name()))
        qWarning() << "CuteIPC:" << "Type doesn't match:" << typeName << "Expected:" << arg.name();

      bool dataLoaded = (type == QMetaType::QImage) ? loadQImage(stream, arg.data()) : QMetaType::load(stream, type, arg.data());
      if (!dataLoaded)
        qWarning() << "CuteIPC:" << "Failed to deserialize argument value" << "of type" << typeName;
    }
  }

  return CuteIPCMessage(type, method, args, returnType);
}


bool CuteIPCMarshaller::marshallArgumentToStream(QGenericArgument value, QDataStream& stream)
{
  // Detect and check type
  int type = QMetaType::type(value.name());
  if (type == 0)
  {
    qWarning() << "CuteIPC:" << "Type" << value.name() << "have not been registered in Qt metaobject system";
    return false;
  }
  if (type == QMetaType::type("QImage"))
    return marshallQImageToStream(value, stream);

  stream << QString::fromLatin1(value.name());
  bool ok = QMetaType::save(stream, type, value.data());
  if (!ok)
  {
    qWarning() << "CuteIPC:" << "Failed to serialize" << value.name()
               << "to data stream. Call qRegisterMetaTypeStreamOperators to"
                  " register stream operators for this metatype";
    return false;
  }

  return true;
}


QGenericArgument CuteIPCMarshaller::demarshallArgumentFromStream(bool& ok, QDataStream& stream)
{
  // Load type
  QString typeName;
  stream >> typeName;

  // Check type
  int type = QMetaType::type(typeName.toLatin1());
  if (type == 0)
  {
    qWarning() << "CuteIPC:" << "Unsupported type of argument " << ":" << typeName;
    ok = false;
    return QGenericArgument();
  }

  // Read argument data from stream
  void* data = QMetaType::construct(type);
  bool dataLoaded = (type == QMetaType::QImage) ? loadQImage(stream, data) : QMetaType::load(stream, type, data);
  if (!dataLoaded)
  {
    qWarning() << "CuteIPC:" << "Failed to deserialize argument value" << "of type" << typeName;
    QMetaType::destroy(type, data);
    ok = false;
    return QGenericArgument();
  }

  ok = true;
  return QGenericArgument(qstrdup(typeName.toLatin1()), data);
}


bool CuteIPCMarshaller::marshallQImageToStream(QGenericArgument value, QDataStream& stream)
{
  QImage* image = (QImage*) value.data();
  const uchar* imageData = image->constBits();

  stream << QString::fromLatin1(value.name());

  stream << image->width();
  stream << image->height();
  stream << image->format();

  stream << image->colorCount();
  if (image->colorCount() > 0)
    stream << image->colorTable();

  stream << image->byteCount();
  stream.writeRawData((const char*)imageData, image->byteCount());
  return true;
}


bool CuteIPCMarshaller::loadQImage(QDataStream& stream, void* data)
{
  // Construct image
  int width;
  stream >> width;
  int height;
  stream >> height;
  int format;
  stream >> format;

  QImage img(width, height, QImage::Format(format));

  // Construct color table (if needed)
  int colorCount;
  stream >> colorCount;

  if (colorCount > 0)
  {
    QVector<QRgb> colorTable;
    stream >> colorTable;
    img.setColorTable(colorTable);
  }

  // Read image bytes
  int byteCount;
  stream >> byteCount;
  if (stream.readRawData(reinterpret_cast<char*>(img.bits()), byteCount) != byteCount)
  {
    qWarning() << "CuteIPC:" << "Failed to deserialize argument value" << "of type" << "QImage";
    return false;
  }

  *static_cast<QImage*>(data) = img;
  return true;
}


void CuteIPCMarshaller::freeArguments(const CuteIPCMessage::Arguments& args)
{
  // Free allocated memory
  for (int i = 0; i < args.size(); ++i)
    freeArgument(args.at(i));
}


void CuteIPCMarshaller::freeArgument(QGenericArgument arg)
{
  if (arg.data())
    QMetaType::destroy(QMetaType::type(arg.name()), arg.data());

  delete[] arg.name();
}
