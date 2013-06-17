#include "carddatabase.h"
#include "settingscache.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QSvgRenderer>
#include <QPainter>
#include <QUrl>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

CardSet::CardSet(const QString &_shortName, const QString &_longName)
	: shortName(_shortName), longName(_longName)
{
	updateSortKey();
}

QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardSet *set)
{
	xml.writeStartElement("set");
	xml.writeTextElement("name", set->getShortName());
	xml.writeTextElement("longname", set->getLongName());
	xml.writeEndElement();

	return xml;
}

void CardSet::setSortKey(unsigned int _sortKey)
{
	sortKey = _sortKey;

	QSettings settings;
	settings.beginGroup("sets");
	settings.beginGroup(shortName);
	settings.setValue("sortkey", sortKey);
}

void CardSet::updateSortKey()
{
	QSettings settings;
	settings.beginGroup("sets");
	settings.beginGroup(shortName);
	sortKey = settings.value("sortkey", 0).toInt();
}

class SetList::CompareFunctor {
public:
	inline bool operator()(CardSet *a, CardSet *b) const
	{
		return a->getSortKey() < b->getSortKey();
	}
};

void SetList::sortByKey()
{
	qSort(begin(), end(), CompareFunctor());
}

PictureLoadingThread::PictureLoadingThread(QObject *parent)
	: QThread(parent)
{
}

PictureLoadingThread::~PictureLoadingThread()
{
	quit();
	wait();
}

void PictureLoadingThread::run()
{
	forever {
		mutex.lock();
		if (loadQueue.isEmpty()) {
			mutex.unlock();
			return;
		}
                QPair<CardInfo*, bool> queueItem = loadQueue.takeFirst();
                CardInfo *card = queueItem.first;
                bool stripped = queueItem.second;
		QString correctedName = card->getCorrectedName();
		QString picsPath = _picsPath;
		SetList sortedSets = card->getSets();
		mutex.unlock();
		
		sortedSets.sortByKey();
                QString suffix = "";
                if (!stripped)
                    suffix = ".full";
		QImage image;
		for (int i = 0; i < sortedSets.size(); i++) {
                        if (image.load(QString("%1/%2/%3%4.jpg").arg(picsPath).arg(sortedSets[i]->getShortName()).arg(correctedName).arg(suffix)))
				break;
                        if (image.load(QString("%1/%2/%3%4%5.jpg").arg(picsPath).arg(sortedSets[i]->getShortName()).arg(correctedName).arg(1).arg(suffix)))
				break;
		}
		if (image.isNull())
                        image.load(QString("%1/%2/%3%4.jpg").arg(picsPath).arg("downloadedPics").arg(correctedName).arg(suffix));
		
                emit imageLoaded(card, image, stripped);
	}
}

void PictureLoadingThread::loadImage(CardInfo *card, bool stripped)
{
	QMutexLocker locker(&mutex);
        QPair<CardInfo*, bool> pair;
        pair.first = card;
        pair.second = stripped;
        loadQueue.append(pair);
	
	if (!isRunning())
		start(LowPriority);
}

void PictureLoadingThread::setPicsPath(const QString &path)
{
	QMutexLocker locker(&mutex);
	_picsPath = path;
}

CardInfo::CardInfo(CardDatabase *_db, const QString &_name, const QString &_manacost, const QString &_cardtype, const QString &_powtough, const QString &_text, const QStringList &_colors, bool _cipt, int _tableRow, const SetList &_sets, const QString &_picURL, const QString &_picHqURL, const QString &_picStURL)
        : db(_db), name(_name), sets(_sets), manacost(_manacost), cardtype(_cardtype), powtough(_powtough), text(_text), colors(_colors), picURL(_picURL), picHqURL(_picHqURL), picStURL(_picStURL), cipt(_cipt), tableRow(_tableRow), pixmap(NULL), pixmapSt(NULL)
{
	for (int i = 0; i < sets.size(); i++)
		sets[i]->append(this);
}

CardInfo::~CardInfo()
{
	clearPixmapCache();
        clearPixmapStCache();
}

QString CardInfo::getMainCardType() const
{
	QString result = getCardType();
	/*
	Legendary Artifact Creature - Golem
	Instant // Instant
	*/

	int pos;
	if ((pos = result.indexOf('-')) != -1)
		result.remove(pos, result.length());
	if ((pos = result.indexOf("//")) != -1)
		result.remove(pos, result.length());
	result = result.simplified();
	/*
	Legendary Artifact Creature
	Instant
	*/

	if ((pos = result.lastIndexOf(' ')) != -1)
		result = result.mid(pos + 1);
	/*
	Creature
	Instant
	*/

	return result;
}

QString CardInfo::getCorrectedName() const
{
	QString result = name;
	// Fire // Ice, Circle of Protection: Red
	return result.remove(" // ").remove(":");
}

void CardInfo::addToSet(CardSet *set)
{
	set->append(this);
	sets << set;
}

QPixmap *CardInfo::loadPixmap(bool stripped)
{
    if (stripped) {
        if (pixmapSt)
                return pixmapSt;
        pixmapSt = new QPixmap();

        if (getName().isEmpty()) {
                pixmapSt->load(settingsCache->getCardBackPicturePath());
                return pixmapSt;
        }
        db->loadImage(this, stripped);

        return pixmapSt;
    }
    else {
        if (pixmap)
                return pixmap;
        pixmap = new QPixmap();

	if (getName().isEmpty()) {
		pixmap->load(settingsCache->getCardBackPicturePath());
		return pixmap;
	}
        db->loadImage(this, stripped);

	return pixmap;
    }
}

void CardInfo::imageLoaded(const QImage &image, bool stripped)
{
	if (!image.isNull()) {
            if (stripped)
		*pixmap = QPixmap::fromImage(image);
            else
                *pixmapSt = QPixmap::fromImage(image);
            emit pixmapUpdated();
	} else if (settingsCache->getPicDownload())
                db->startPicDownload(this, stripped);
}

QPixmap *CardInfo::getPixmap(QSize size, bool stripped)
{
	qDebug(QString("CardInfo::getPixmap(%1, %2) for %3").arg(size.width()).arg(size.height()).arg(getName()).toLatin1());
        QPixmap *cachedPixmap;
        if (stripped)
            cachedPixmap = scaledPixmapStCache.value(size.width());
        else
            cachedPixmap = scaledPixmapCache.value(size.width());
	if (cachedPixmap)
		return cachedPixmap;
        QPixmap *bigPixmap = loadPixmap(stripped);
	QPixmap *result;
	if (bigPixmap->isNull()) {
		if (!getName().isEmpty())
			return 0;
		else {
			result = new QPixmap(size);
			result->fill(Qt::transparent);
			QSvgRenderer svg(QString(":/back.svg"));
			QPainter painter(result);
			svg.render(&painter, QRectF(0, 0, size.width(), size.height()));
		}
	} else
		result = new QPixmap(bigPixmap->scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        if (stripped)
            scaledPixmapCache.insert(size.width(), result);
        else
            scaledPixmapStCache.insert(size.width(), result);
	return result;
}

void CardInfo::clearPixmapCache()
{
	if (pixmap) {
		qDebug(QString("Deleting pixmap for %1").arg(name).toLatin1());
		delete pixmap;
		pixmap = 0;
		QMapIterator<int, QPixmap *> i(scaledPixmapCache);
		while (i.hasNext()) {
			i.next();
			qDebug(QString("  Deleting cached pixmap for width %1").arg(i.key()).toLatin1());
			delete i.value();
		}
                scaledPixmapCache.clear();
	}
}


void CardInfo::clearPixmapStCache()
{
        if (pixmapSt) {
                qDebug(QString("Deleting pixmap stripped for %1").arg(name).toLatin1());
                delete pixmapSt;
                pixmapSt = 0;
                QMapIterator<int, QPixmap *> i(scaledPixmapStCache);
                while (i.hasNext()) {
                        i.next();
                        qDebug(QString("  Deleting cached pixmap for width %1").arg(i.key()).toLatin1());
                        delete i.value();
                }
                scaledPixmapStCache.clear();
        }
}

void CardInfo::clearPixmapCacheMiss()
{
	if (!pixmap)
		return;
	if (pixmap->isNull())
		clearPixmapCache();
        if (!pixmapSt)
                return;
        if (pixmapSt->isNull())
                clearPixmapStCache();
}

void CardInfo::updatePixmapCache(bool stripped)
{
	qDebug(QString("Updating pixmap cache for %1").arg(name).toLatin1());
        if (stripped)
            clearPixmapStCache();
        else
            clearPixmapCache();
        loadPixmap(stripped);
	
	emit pixmapUpdated();
}

QXmlStreamWriter &operator<<(QXmlStreamWriter &xml, const CardInfo *info)
{
	xml.writeStartElement("card");
	xml.writeTextElement("name", info->getName());

	const SetList &sets = info->getSets();
	for (int i = 0; i < sets.size(); i++)
		xml.writeTextElement("set", sets[i]->getShortName());
	const QStringList &colors = info->getColors();
	for (int i = 0; i < colors.size(); i++)
		xml.writeTextElement("color", colors[i]);

	xml.writeTextElement("manacost", info->getManaCost());
	xml.writeTextElement("type", info->getCardType());
	if (!info->getPowTough().isEmpty())
		xml.writeTextElement("pt", info->getPowTough());
	xml.writeTextElement("tablerow", QString::number(info->getTableRow()));
	xml.writeTextElement("text", info->getText());
	xml.writeTextElement("picURL", info->getPicURL());
        xml.writeTextElement("picHqURL", info->getPicHqURL());
        xml.writeTextElement("picStURL", info->getPicStURL());
	if (info->getCipt())
		xml.writeTextElement("cipt", "1");
	xml.writeEndElement(); // card

	return xml;
}

CardDatabase::CardDatabase(QObject *parent)
        : QObject(parent), downloadRunning(false), failLQ(false), loadSuccess(false), noCard(0)
{
	connect(settingsCache, SIGNAL(picsPathChanged()), this, SLOT(clearPixmapCache()));
        connect(settingsCache, SIGNAL(picsPathChanged()), this, SLOT(clearPixmapStCache()));
	connect(settingsCache, SIGNAL(cardDatabasePathChanged()), this, SLOT(loadCardDatabase()));
	connect(settingsCache, SIGNAL(picDownloadChanged()), this, SLOT(picDownloadChanged()));
	
	networkManager = new QNetworkAccessManager(this);
	connect(networkManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(picDownloadFinished(QNetworkReply *)));

	loadCardDatabase();
	
	loadingThread = new PictureLoadingThread(this);
	loadingThread->setPicsPath(settingsCache->getPicsPath());
        connect(loadingThread, SIGNAL(imageLoaded(CardInfo *, QImage, bool)), this, SLOT(imageLoaded(CardInfo *, QImage, bool)));
	loadingThread->start();

	noCard = new CardInfo(this);
        noCard->loadPixmap(false); // cache pixmap for card back
	connect(settingsCache, SIGNAL(cardBackPicturePathChanged()), noCard, SLOT(updatePixmapCache()));
}

CardDatabase::~CardDatabase()
{
	clear();
	delete noCard;
}

void CardDatabase::clear()
{
	QHashIterator<QString, CardSet *> setIt(setHash);
	while (setIt.hasNext()) {
		setIt.next();
		delete setIt.value();
	}
	setHash.clear();

	QHashIterator<QString, CardInfo *> i(cardHash);
	while (i.hasNext()) {
		i.next();
		delete i.value();
	}
	cardHash.clear();
}

CardInfo *CardDatabase::getCard(const QString &cardName)
{
	if (cardName.isEmpty())
		return noCard;
	else if (cardHash.contains(cardName))
		return cardHash.value(cardName);
	else {
		qDebug(QString("CardDatabase: card not found: %1").arg(cardName).toLatin1());
		CardInfo *newCard = new CardInfo(this, cardName);
		newCard->addToSet(getSet("TK"));
		cardHash.insert(cardName, newCard);
		return newCard;
	}
}

CardSet *CardDatabase::getSet(const QString &setName)
{
	if (setHash.contains(setName))
		return setHash.value(setName);
	else {
		qDebug(QString("CardDatabase: set not found: %1").arg(setName).toLatin1());
		CardSet *newSet = new CardSet(setName);
		setHash.insert(setName, newSet);
		return newSet;
	}
}

SetList CardDatabase::getSetList() const
{
	SetList result;
	QHashIterator<QString, CardSet *> i(setHash);
	while (i.hasNext()) {
		i.next();
		result << i.value();
	}
	return result;
}

void CardDatabase::clearPixmapCache()
{
	QHashIterator<QString, CardInfo *> i(cardHash);
	while (i.hasNext()) {
		i.next();
		i.value()->clearPixmapCache();
	}
	if (noCard)
		noCard->clearPixmapCache();
}

void CardDatabase::clearPixmapStCache()
{
        QHashIterator<QString, CardInfo *> i(cardHash);
        while (i.hasNext()) {
                i.next();
                i.value()->clearPixmapStCache();
        }
        if (noCard)
                noCard->clearPixmapStCache();
}

void CardDatabase::startPicDownload(CardInfo *card, bool stripped)
{
	if (card->getPicURL().isEmpty())
		return;
	
        cardsToDownload.append(qMakePair(card, stripped));
	if (!downloadRunning)
		startNextPicDownload();
}

void CardDatabase::startNextPicDownload()
{
        if (cardsToDownload.isEmpty()) { //FIXME
                //cardBeingDownloaded = qMakePair();
		downloadRunning = false;
		return;
	}
	
	downloadRunning = true;
        failLQ = false;
	cardBeingDownloaded = cardsToDownload.takeFirst();
        if (!cardBeingDownloaded.second){
            QNetworkRequest req(QUrl(cardBeingDownloaded.first->getPicHqURL()));
            networkManager->get(req);
        }
        else {
            QNetworkRequest req(QUrl(cardBeingDownloaded.first->getPicStURL()));
            networkManager->get(req);
        }

}

void CardDatabase::picDownloadFinished(QNetworkReply *reply)
{
	QString picsPath = settingsCache->getPicsPath();
	const QByteArray &picData = reply->readAll();
	QPixmap testPixmap;
	if (testPixmap.loadFromData(picData)) {
		if (!QDir(QString(picsPath + "/downloadedPics/")).exists()) {
			QDir dir(picsPath);
			if (!dir.exists())
				return;
			dir.mkdir("downloadedPics");
		}
                QString suffix = "";
                if (!cardBeingDownloaded.second)
                    suffix = ".full";
                QFile newPic(picsPath + "/downloadedPics/" + cardBeingDownloaded.first->getCorrectedName() + suffix + ".jpg");
		if (!newPic.open(QIODevice::WriteOnly))
			return;
		newPic.write(picData);
		newPic.close();
		
                cardBeingDownloaded.first->updatePixmapCache(cardBeingDownloaded.second);

                reply->deleteLater();
                startNextPicDownload();
	}
        else if (!failLQ && !cardBeingDownloaded.second) {
            failLQ = true;
            QNetworkRequest req(QUrl(cardBeingDownloaded.first->getPicURL()));
            networkManager->get(req);
        }
        else {
            reply->deleteLater();
            startNextPicDownload();
        }
}

void CardDatabase::loadSetsFromXml(QXmlStreamReader &xml)
{
	while (!xml.atEnd()) {
		if (xml.readNext() == QXmlStreamReader::EndElement)
			break;
		if (xml.name() == "set") {
			QString shortName, longName;
			while (!xml.atEnd()) {
				if (xml.readNext() == QXmlStreamReader::EndElement)
					break;
				if (xml.name() == "name")
					shortName = xml.readElementText();
				else if (xml.name() == "longname")
					longName = xml.readElementText();
			}
			setHash.insert(shortName, new CardSet(shortName, longName));
		}
	}
}

void CardDatabase::loadCardsFromXml(QXmlStreamReader &xml)
{
	while (!xml.atEnd()) {
		if (xml.readNext() == QXmlStreamReader::EndElement)
			break;
		if (xml.name() == "card") {
                        QString name, manacost, type, pt, text, picURL, picHqURL, picStURL;
			QStringList colors;
			SetList sets;
			int tableRow = 0;
			bool cipt = false;
			while (!xml.atEnd()) {
				if (xml.readNext() == QXmlStreamReader::EndElement)
					break;
				if (xml.name() == "name")
					name = xml.readElementText();
				else if (xml.name() == "manacost")
					manacost = xml.readElementText();
				else if (xml.name() == "type")
					type = xml.readElementText();
				else if (xml.name() == "pt")
					pt = xml.readElementText();
				else if (xml.name() == "text")
					text = xml.readElementText();
				else if (xml.name() == "set")
					sets << getSet(xml.readElementText());
				else if (xml.name() == "color")
					colors << xml.readElementText();
				else if (xml.name() == "tablerow")
					tableRow = xml.readElementText().toInt();
				else if (xml.name() == "picURL")
					picURL = xml.readElementText();
                                else if (xml.name() == "picHqURL")
                                        picHqURL = xml.readElementText();
                                else if (xml.name() == "picStURL")
                                        picStURL = xml.readElementText();
				else if (xml.name() == "cipt")
					cipt = (xml.readElementText() == "1");
			}
                        cardHash.insert(name, new CardInfo(this, name, manacost, type, pt, text, colors, cipt, tableRow, sets, picURL, picHqURL, picStURL));
		}
	}
}

bool CardDatabase::loadFromFile(const QString &fileName)
{
	QFile file(fileName);
	file.open(QIODevice::ReadOnly);
	if (!file.isOpen())
		return false;
	QXmlStreamReader xml(&file);
	clear();
	while (!xml.atEnd()) {
		if (xml.readNext() == QXmlStreamReader::StartElement) {
			if (xml.name() != "cockatrice_carddatabase")
				return false;
			while (!xml.atEnd()) {
				if (xml.readNext() == QXmlStreamReader::EndElement)
					break;
				if (xml.name() == "sets")
					loadSetsFromXml(xml);
				else if (xml.name() == "cards")
					loadCardsFromXml(xml);
			}
		}
	}
	qDebug(QString("%1 cards in %2 sets loaded").arg(cardHash.size()).arg(setHash.size()).toLatin1());
	return !cardHash.isEmpty();
}

bool CardDatabase::saveToFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	QXmlStreamWriter xml(&file);

	xml.setAutoFormatting(true);
	xml.writeStartDocument();
	xml.writeStartElement("cockatrice_carddatabase");
	xml.writeAttribute("version", "1");

	xml.writeStartElement("sets");
	QHashIterator<QString, CardSet *> setIterator(setHash);
	while (setIterator.hasNext())
		xml << setIterator.next().value();
	xml.writeEndElement(); // sets

	xml.writeStartElement("cards");
	QHashIterator<QString, CardInfo *> cardIterator(cardHash);
	while (cardIterator.hasNext())
		xml << cardIterator.next().value();
	xml.writeEndElement(); // cards

	xml.writeEndElement(); // cockatrice_carddatabase
	xml.writeEndDocument();

	return true;
}

void CardDatabase::picDownloadChanged()
{
	if (settingsCache->getPicDownload()) {
		QHashIterator<QString, CardInfo *> cardIterator(cardHash);
		while (cardIterator.hasNext())
			cardIterator.next().value()->clearPixmapCacheMiss();
	}
}

bool CardDatabase::loadCardDatabase(const QString &path)
{
	if (!path.isEmpty())
		loadSuccess = loadFromFile(path);
	else loadSuccess = false;
	return loadSuccess;
}

bool CardDatabase::loadCardDatabase()
{
	return loadCardDatabase(settingsCache->getCardDatabasePath());
}

QStringList CardDatabase::getAllColors() const
{
	QSet<QString> colors;
	QHashIterator<QString, CardInfo *> cardIterator(cardHash);
	while (cardIterator.hasNext()) {
		const QStringList &cardColors = cardIterator.next().value()->getColors();
		if (cardColors.isEmpty())
			colors.insert("X");
		else
			for (int i = 0; i < cardColors.size(); ++i)
				colors.insert(cardColors[i]);
	}
	return colors.toList();
}

QStringList CardDatabase::getAllMainCardTypes() const
{
	QSet<QString> types;
	QHashIterator<QString, CardInfo *> cardIterator(cardHash);
	while (cardIterator.hasNext())
		types.insert(cardIterator.next().value()->getMainCardType());
	return types.toList();
}

void CardDatabase::cacheCardPixmaps(const QStringList &cardNames)
{
    for (int i = 0; i < cardNames.size(); ++i) {
            //FIXME
                getCard(cardNames[i])->loadPixmap(true);
                getCard(cardNames[i])->loadPixmap(false);
            }

}

void CardDatabase::loadImage(CardInfo *card, bool stripped)
{
        loadingThread->loadImage(card, stripped);
}

void CardDatabase::imageLoaded(CardInfo *card, QImage image, bool stripped)
{
        card->imageLoaded(image, stripped);
}
