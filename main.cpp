/*
* Author:  Luca Carlon
* Company: -
* Date:    14.12.2013
*/

/*------------------------------------------------------------------------------
|    includes
+-----------------------------------------------------------------------------*/
#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QUrl>

#include <lc_logging.h>

#include <mpegfile.h>
#include <tag.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v1tag.h>
#include <popularimeterframe.h>
#include <tstring.h>

/*------------------------------------------------------------------------------
|    definitions
+-----------------------------------------------------------------------------*/
using namespace TagLib;

QString libFilePath;
QString email;

/*------------------------------------------------------------------------------
|    print_usage
+-----------------------------------------------------------------------------*/
bool print_usage(QStringList args)
{
   log_info("Usage: %s <iTunes_xml_lib> <user>", qPrintable(args[0]));
   log_info("<iTunes_xml_lib>: path to the library xml;");
   log_info("<user>          : email of the user (this email address will be associated to popularimeter data.");

   return false;
}

/*------------------------------------------------------------------------------
|    validate_args
+-----------------------------------------------------------------------------*/
bool validate_args(QStringList args)
{
   if (args.size() < 3)
      return print_usage(args);

   libFilePath = args[1];
   email       = args[2];

   return true;
}

/*------------------------------------------------------------------------------
|    write_popularimeter_to_mp3
+-----------------------------------------------------------------------------*/
bool write_popularimeter_to_mp3(QString mp3Path, int counter, int rating)
{
   MPEG::File f(mp3Path.toLocal8Bit().data(), true);

   // If no ID3v2 tag then get v1 and duplicate.
   ID3v2::Tag* tagv2 = NULL;
   if (!(tagv2 = f.ID3v2Tag(false))) {
      ID3v1::Tag* tagv1 = NULL;
      tagv2 = f.ID3v2Tag(true);
      if ((tagv1 = f.ID3v1Tag())) {
         log_info("Created ID3v2 tags from ID3v1.");
         TagLib::Tag::duplicate(tagv1, tagv2, true);
      }

      log_info("Created empty ID3v2 tags.");
   }
   else {
      log_verbose("ID3v2 tags found.");

      log_debug("Frame list:");
      ID3v2::FrameList list = f.ID3v2Tag()->frameList();
      for (uint i = 0; i < list.size(); i++)
         log_debug("Frame: %s, %s.", String(list[i]->frameID()).toCString(), list[i]->toString().toCString());

      // Remove any existing POPM frame with the specific email.
      ID3v2::FrameList popms = f.ID3v2Tag()->frameList("POPM");
      for (uint i = 0; i < popms.size(); i++)
         if (ID3v2::PopularimeterFrame* popm = dynamic_cast<ID3v2::PopularimeterFrame*>(popms[0]))
            f.ID3v2Tag()->removeFrame(popm);
   }

   // Create a new frame and overwrite.
   ID3v2::PopularimeterFrame* frame = new ID3v2::PopularimeterFrame();
   frame->setCounter(counter);
   frame->setEmail(String(email.toLocal8Bit().data()));
   frame->setRating(rating);
   tagv2->addFrame(frame);

   if (!f.save()) {
      log_err("Failed to save changes.");
      return false;
   }

   return log_verbose("Changes were saved.");
}

/*------------------------------------------------------------------------------
|    write_popularimeter
+-----------------------------------------------------------------------------*/
bool write_popularimeter()
{
   QFile xml(libFilePath);
   if (!xml.open(QIODevice::ReadOnly))
      return log_err("Failed to open library XML.");

   QString urlString;
   int playCount = 0;
   int rating    = 0;
   bool convOk;

   // Parse.
   QXmlStreamReader reader(&xml);
   while (!reader.atEnd()) {
      switch (reader.readNext()) {
      case QXmlStreamReader::StartElement: {
         if (reader.name() != "key")
            break;

         if (reader.readNext() != QXmlStreamReader::Characters)
            continue;
         QString elementType = reader.text().toString();
         if (reader.readNext() != QXmlStreamReader::EndElement)
            continue;
         if (reader.readNext() != QXmlStreamReader::StartElement)
            continue;
         if (reader.readNext() != QXmlStreamReader::Characters)
            continue;

         if (elementType == "Play Count") {
            QString scount = reader.text().toString();
            playCount = scount.toInt(&convOk);
            if (!convOk)
               return false;
         }
         else if (elementType == "Rating") {
            QString srating = reader.text().toString();
            rating = (int)(srating.toInt(&convOk)/100.0*255);
            if (!convOk)
               return false;
         }
         else if (elementType == "Location") {
            urlString = reader.text().toString();
            if (urlString.isNull())
               return false;
         }

         break;
      }
      case QXmlStreamReader::EndElement: {
         if (reader.name().toString() != "dict")
            break;
         if (urlString.isNull())
            break;

         QUrl url = QUrl::fromPercentEncoding(urlString.toLocal8Bit());
         QFileInfo file(url.toLocalFile().replace("/localhost", ""));

         if (!file.exists())
            log_warn("Failed to find file %s.", qPrintable(file.absoluteFilePath()));
         else {
				log_info("Writing info (%d, %d) to %s.", playCount, rating, qPrintable(file.absoluteFilePath()));
            if (!write_popularimeter_to_mp3(file.absoluteFilePath(), playCount, rating))
               return false;
         }

         urlString = QString();
         playCount   = 0;
         rating      = 0;

         break;
      }
      default:
         break;
      }

      if (reader.hasError()) {
         log_err("XML error: %s.", qPrintable(reader.errorString()));
         return false;
      }
   }

   return true;
}

/*------------------------------------------------------------------------------
|    main
+-----------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
   QCoreApplication a(argc, argv);

   QStringList args = a.arguments();
   if (!validate_args(args))
      return 1;
   if (!write_popularimeter())
      return 1;

   return 0;
}
