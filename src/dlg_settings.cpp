#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QToolBar>
#include <QTranslator>
#include <QAction>
#include <QSettings>
#include <QApplication>
#include <QInputDialog>
#include "carddatabase.h"
#include "dlg_settings.h"
#include "main.h"
#include "settingscache.h"

GeneralSettingsPage::GeneralSettingsPage()
{
	languageLabel = new QLabel;
	languageBox = new QComboBox;
	
	QString setLanguage = settingsCache->getLang();
	QStringList qmFiles = findQmFiles();
	for (int i = 0; i < qmFiles.size(); i++) {
		QString langName = languageName(qmFiles[i]);
		languageBox->addItem(langName, qmFiles[i]);
		if ((qmFiles[i] == setLanguage) || (setLanguage.isEmpty() && langName == tr("English")))
			languageBox->setCurrentIndex(i);
	}
	
	picDownloadCheckBox = new QCheckBox;
	picDownloadCheckBox->setChecked(settingsCache->getPicDownload());
	
	connect(languageBox, SIGNAL(currentIndexChanged(int)), this, SLOT(languageBoxChanged(int)));
	connect(picDownloadCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setPicDownload(int)));
	
	QGridLayout *personalGrid = new QGridLayout;
	personalGrid->addWidget(languageLabel, 0, 0);
	personalGrid->addWidget(languageBox, 0, 1);
	personalGrid->addWidget(picDownloadCheckBox, 1, 0, 1, 2);
	
	personalGroupBox = new QGroupBox;
	personalGroupBox->setLayout(personalGrid);
	
	deckPathLabel = new QLabel;
	deckPathEdit = new QLineEdit(settingsCache->getDeckPath());
	deckPathEdit->setReadOnly(true);
	QPushButton *deckPathButton = new QPushButton("...");
	connect(deckPathButton, SIGNAL(clicked()), this, SLOT(deckPathButtonClicked()));
	
	picsPathLabel = new QLabel;
	picsPathEdit = new QLineEdit(settingsCache->getPicsPath());
	picsPathEdit->setReadOnly(true);
	QPushButton *picsPathButton = new QPushButton("...");
	connect(picsPathButton, SIGNAL(clicked()), this, SLOT(picsPathButtonClicked()));
	
	cardDatabasePathLabel = new QLabel;
	cardDatabasePathEdit = new QLineEdit(settingsCache->getCardDatabasePath());
	cardDatabasePathEdit->setReadOnly(true);
	QPushButton *cardDatabasePathButton = new QPushButton("...");
	connect(cardDatabasePathButton, SIGNAL(clicked()), this, SLOT(cardDatabasePathButtonClicked()));
	
	QGridLayout *pathsGrid = new QGridLayout;
	pathsGrid->addWidget(deckPathLabel, 0, 0);
	pathsGrid->addWidget(deckPathEdit, 0, 1);
	pathsGrid->addWidget(deckPathButton, 0, 2);
	pathsGrid->addWidget(picsPathLabel, 1, 0);
	pathsGrid->addWidget(picsPathEdit, 1, 1);
	pathsGrid->addWidget(picsPathButton, 1, 2);
	pathsGrid->addWidget(cardDatabasePathLabel, 2, 0);
	pathsGrid->addWidget(cardDatabasePathEdit, 2, 1);
	pathsGrid->addWidget(cardDatabasePathButton, 2, 2);
	pathsGroupBox = new QGroupBox;
	pathsGroupBox->setLayout(pathsGrid);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(personalGroupBox);
	mainLayout->addWidget(pathsGroupBox);
	
	setLayout(mainLayout);
}

QStringList GeneralSettingsPage::findQmFiles()
{
	QDir dir(":/translations");
	QStringList fileNames = dir.entryList(QStringList(translationPrefix + "_*.qm"), QDir::Files, QDir::Name);
	fileNames.replaceInStrings(QRegExp(translationPrefix + "_(.*)\\.qm"), "\\1");
	return fileNames;
}

QString GeneralSettingsPage::languageName(const QString &qmFile)
{
	QTranslator translator;
	translator.load(":/translations/" + translationPrefix + "_" + qmFile + ".qm");
	
	return translator.translate("GeneralSettingsPage", "English");
}

void GeneralSettingsPage::deckPathButtonClicked()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	deckPathEdit->setText(path);
	settingsCache->setDeckPath(path);
}

void GeneralSettingsPage::picsPathButtonClicked()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	picsPathEdit->setText(path);
	settingsCache->setPicsPath(path);
}

void GeneralSettingsPage::cardDatabasePathButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	cardDatabasePathEdit->setText(path);
	settingsCache->setCardDatabasePath(path);
}

void GeneralSettingsPage::languageBoxChanged(int index)
{
	settingsCache->setLang(languageBox->itemData(index).toString());
}

void GeneralSettingsPage::retranslateUi()
{
	personalGroupBox->setTitle(tr("Personal settings"));
	languageLabel->setText(tr("Language:"));
	picDownloadCheckBox->setText(tr("Download card pictures on the fly"));
	pathsGroupBox->setTitle(tr("Paths"));
	deckPathLabel->setText(tr("Decks directory:"));
	picsPathLabel->setText(tr("Pictures directory:"));
	cardDatabasePathLabel->setText(tr("Path to card database:"));
}

AppearanceSettingsPage::AppearanceSettingsPage()
{
	QIcon deleteIcon(":/resources/icon_delete.svg");
	
	handBgLabel = new QLabel;
	handBgEdit = new QLineEdit(settingsCache->getHandBgPath());
	handBgEdit->setReadOnly(true);
	QPushButton *handBgClearButton = new QPushButton(deleteIcon, QString());
	connect(handBgClearButton, SIGNAL(clicked()), this, SLOT(handBgClearButtonClicked()));
	QPushButton *handBgButton = new QPushButton("...");
	connect(handBgButton, SIGNAL(clicked()), this, SLOT(handBgButtonClicked()));
	
	stackBgLabel = new QLabel;
	stackBgEdit = new QLineEdit(settingsCache->getStackBgPath());
	stackBgEdit->setReadOnly(true);
	QPushButton *stackBgClearButton = new QPushButton(deleteIcon, QString());
	connect(stackBgClearButton, SIGNAL(clicked()), this, SLOT(stackBgClearButtonClicked()));
	QPushButton *stackBgButton = new QPushButton("...");
	connect(stackBgButton, SIGNAL(clicked()), this, SLOT(stackBgButtonClicked()));

	tableBgLabel = new QLabel;
	tableBgEdit = new QLineEdit(settingsCache->getTableBgPath());
	tableBgEdit->setReadOnly(true);
	QPushButton *tableBgClearButton = new QPushButton(deleteIcon, QString());
	connect(tableBgClearButton, SIGNAL(clicked()), this, SLOT(tableBgClearButtonClicked()));
	QPushButton *tableBgButton = new QPushButton("...");
	connect(tableBgButton, SIGNAL(clicked()), this, SLOT(tableBgButtonClicked()));
	
	playerAreaBgLabel = new QLabel;
	playerAreaBgEdit = new QLineEdit(settingsCache->getPlayerBgPath());
	playerAreaBgEdit->setReadOnly(true);
	QPushButton *playerAreaBgClearButton = new QPushButton(deleteIcon, QString());
	connect(playerAreaBgClearButton, SIGNAL(clicked()), this, SLOT(playerAreaBgClearButtonClicked()));
	QPushButton *playerAreaBgButton = new QPushButton("...");
	connect(playerAreaBgButton, SIGNAL(clicked()), this, SLOT(playerAreaBgButtonClicked()));
	
	cardBackPicturePathLabel = new QLabel;
	cardBackPicturePathEdit = new QLineEdit(settingsCache->getCardBackPicturePath());
	cardBackPicturePathEdit->setReadOnly(true);
	QPushButton *cardBackPicturePathClearButton = new QPushButton(deleteIcon, QString());
	connect(cardBackPicturePathClearButton, SIGNAL(clicked()), this, SLOT(cardBackPicturePathClearButtonClicked()));
	QPushButton *cardBackPicturePathButton = new QPushButton("...");
	connect(cardBackPicturePathButton, SIGNAL(clicked()), this, SLOT(cardBackPicturePathButtonClicked()));
	
	QGridLayout *zoneBgGrid = new QGridLayout;
	zoneBgGrid->addWidget(handBgLabel, 0, 0);
	zoneBgGrid->addWidget(handBgEdit, 0, 1);
	zoneBgGrid->addWidget(handBgClearButton, 0, 2);
	zoneBgGrid->addWidget(handBgButton, 0, 3);
	zoneBgGrid->addWidget(stackBgLabel, 1, 0);
	zoneBgGrid->addWidget(stackBgEdit, 1, 1);
	zoneBgGrid->addWidget(stackBgClearButton, 1, 2);
	zoneBgGrid->addWidget(stackBgButton, 1, 3);
	zoneBgGrid->addWidget(tableBgLabel, 2, 0);
	zoneBgGrid->addWidget(tableBgEdit, 2, 1);
	zoneBgGrid->addWidget(tableBgClearButton, 2, 2);
	zoneBgGrid->addWidget(tableBgButton, 2, 3);
	zoneBgGrid->addWidget(playerAreaBgLabel, 3, 0);
	zoneBgGrid->addWidget(playerAreaBgEdit, 3, 1);
	zoneBgGrid->addWidget(playerAreaBgClearButton, 3, 2);
	zoneBgGrid->addWidget(playerAreaBgButton, 3, 3);
	zoneBgGrid->addWidget(cardBackPicturePathLabel, 4, 0);
	zoneBgGrid->addWidget(cardBackPicturePathEdit, 4, 1);
	zoneBgGrid->addWidget(cardBackPicturePathClearButton, 4, 2);
	zoneBgGrid->addWidget(cardBackPicturePathButton, 4, 3);

	zoneBgGroupBox = new QGroupBox;
	zoneBgGroupBox->setLayout(zoneBgGrid);

	horizontalHandCheckBox = new QCheckBox;
	horizontalHandCheckBox->setChecked(settingsCache->getHorizontalHand());
	connect(horizontalHandCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setHorizontalHand(int)));
	
	QGridLayout *handGrid = new QGridLayout;
	handGrid->addWidget(horizontalHandCheckBox, 0, 0, 1, 2);
	
	handGroupBox = new QGroupBox;
	handGroupBox->setLayout(handGrid);
	
	economicalGridCheckBox = new QCheckBox;
	economicalGridCheckBox->setChecked(settingsCache->getEconomicalGrid());
	connect(economicalGridCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setEconomicalGrid(int)));
	
	invertVerticalCoordinateCheckBox = new QCheckBox;
	invertVerticalCoordinateCheckBox->setChecked(settingsCache->getInvertVerticalCoordinate());
	connect(invertVerticalCoordinateCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setInvertVerticalCoordinate(int)));
	
	QGridLayout *tableGrid = new QGridLayout;
	tableGrid->addWidget(economicalGridCheckBox, 0, 0, 1, 2);
	tableGrid->addWidget(invertVerticalCoordinateCheckBox, 1, 0, 1, 2);
	
	tableGroupBox = new QGroupBox;
	tableGroupBox->setLayout(tableGrid);
	
	zoneViewSortByNameCheckBox = new QCheckBox;
	zoneViewSortByNameCheckBox->setChecked(settingsCache->getZoneViewSortByName());
	connect(zoneViewSortByNameCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setZoneViewSortByName(int)));
	zoneViewSortByTypeCheckBox = new QCheckBox;
	zoneViewSortByTypeCheckBox->setChecked(settingsCache->getZoneViewSortByType());
	connect(zoneViewSortByTypeCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setZoneViewSortByType(int)));

	QGridLayout *zoneViewGrid = new QGridLayout;
	zoneViewGrid->addWidget(zoneViewSortByNameCheckBox, 0, 0, 1, 2);
	zoneViewGrid->addWidget(zoneViewSortByTypeCheckBox, 1, 0, 1, 2);

	zoneViewGroupBox = new QGroupBox;
	zoneViewGroupBox->setLayout(zoneViewGrid);
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(zoneBgGroupBox);
	mainLayout->addWidget(handGroupBox);
	mainLayout->addWidget(tableGroupBox);
	mainLayout->addWidget(zoneViewGroupBox);
	
	setLayout(mainLayout);
}

void AppearanceSettingsPage::retranslateUi()
{
	zoneBgGroupBox->setTitle(tr("Zone background pictures"));
	handBgLabel->setText(tr("Path to hand background:"));
	stackBgLabel->setText(tr("Path to stack background:"));
	tableBgLabel->setText(tr("Path to table background:"));
	playerAreaBgLabel->setText(tr("Path to player info background:"));
	cardBackPicturePathLabel->setText(tr("Path to picture of card back:"));
	
	handGroupBox->setTitle(tr("Hand layout"));
	horizontalHandCheckBox->setText(tr("Display hand horizontally (wastes space)"));
	
	tableGroupBox->setTitle(tr("Table grid layout"));
	economicalGridCheckBox->setText(tr("Economical layout"));
	invertVerticalCoordinateCheckBox->setText(tr("Invert vertical coordinate"));
	
	zoneViewGroupBox->setTitle(tr("Zone view layout"));
	zoneViewSortByNameCheckBox->setText(tr("Sort by name"));
	zoneViewSortByTypeCheckBox->setText(tr("Sort by type"));
}

void AppearanceSettingsPage::handBgClearButtonClicked()
{
	handBgEdit->setText(QString());
	settingsCache->setHandBgPath(QString());
}

void AppearanceSettingsPage::handBgButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	handBgEdit->setText(path);
	settingsCache->setHandBgPath(path);
}

void AppearanceSettingsPage::stackBgClearButtonClicked()
{
	stackBgEdit->setText(QString());
	settingsCache->setStackBgPath(QString());
}

void AppearanceSettingsPage::stackBgButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	stackBgEdit->setText(path);
	settingsCache->setStackBgPath(path);
}

void AppearanceSettingsPage::tableBgClearButtonClicked()
{
	tableBgEdit->setText(QString());
	settingsCache->setTableBgPath(QString());
}

void AppearanceSettingsPage::tableBgButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;

	tableBgEdit->setText(path);
	settingsCache->setTableBgPath(path);
}

void AppearanceSettingsPage::playerAreaBgClearButtonClicked()
{
	playerAreaBgEdit->setText(QString());
	settingsCache->setPlayerBgPath(QString());
}

void AppearanceSettingsPage::playerAreaBgButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	playerAreaBgEdit->setText(path);
	settingsCache->setPlayerBgPath(path);
}

void AppearanceSettingsPage::cardBackPicturePathClearButtonClicked()
{
	cardBackPicturePathEdit->setText(QString());
	settingsCache->setCardBackPicturePath(QString());
}

void AppearanceSettingsPage::cardBackPicturePathButtonClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Choose path"));
	if (path.isEmpty())
		return;
	
	cardBackPicturePathEdit->setText(path);
	settingsCache->setCardBackPicturePath(path);
}

UserInterfaceSettingsPage::UserInterfaceSettingsPage()
{
	doubleClickToPlayCheckBox = new QCheckBox;
	doubleClickToPlayCheckBox->setChecked(settingsCache->getDoubleClickToPlay());
	connect(doubleClickToPlayCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setDoubleClickToPlay(int)));
	
        cardInfoFontSizeLabel = new QLabel;
        cardInfoFontSizeSpinBox = new QSpinBox;
        cardInfoFontSizeSpinBox->setValue(settingsCache->getCardInfoFontSize());
        connect(cardInfoFontSizeSpinBox, SIGNAL(valueChanged(int)), settingsCache, SLOT(setCardInfoFontSize(int)));


        cardInfoStrippedCheckBox = new QCheckBox;
        cardInfoStrippedCheckBox->setChecked(settingsCache->getCardInfoStripped());
        connect(cardInfoStrippedCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setCardInfoStripped(int)));

	QGridLayout *generalGrid = new QGridLayout;
	generalGrid->addWidget(doubleClickToPlayCheckBox, 0, 0);
        generalGrid->addWidget(cardInfoFontSizeLabel, 1, 0);
        generalGrid->addWidget(cardInfoFontSizeSpinBox, 1, 1);
        generalGrid->addWidget(cardInfoStrippedCheckBox, 2, 0);
	
	generalGroupBox = new QGroupBox;
	generalGroupBox->setLayout(generalGrid);
	
	tapAnimationCheckBox = new QCheckBox;
	tapAnimationCheckBox->setChecked(settingsCache->getTapAnimation());
	connect(tapAnimationCheckBox, SIGNAL(stateChanged(int)), settingsCache, SLOT(setTapAnimation(int)));
	
	QGridLayout *animationGrid = new QGridLayout;
	animationGrid->addWidget(tapAnimationCheckBox, 0, 0);
	
	animationGroupBox = new QGroupBox;
	animationGroupBox->setLayout(animationGrid);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(generalGroupBox);
	mainLayout->addWidget(animationGroupBox);
	
	setLayout(mainLayout);
}

void UserInterfaceSettingsPage::retranslateUi()
{
	generalGroupBox->setTitle(tr("General interface settings"));
	doubleClickToPlayCheckBox->setText(tr("&Double-click cards to play them (instead of single-click)"));
	animationGroupBox->setTitle(tr("Animation settings"));
	tapAnimationCheckBox->setText(tr("&Tap/untap animation"));
        cardInfoFontSizeLabel->setText(tr("Card info font size:"));
        cardInfoStrippedCheckBox->setText(tr("Show stripped card picture on card info frame"));
}

MessagesSettingsPage::MessagesSettingsPage()
{
	aAdd = new QAction(this);
	connect(aAdd, SIGNAL(triggered()), this, SLOT(actAdd()));
	aRemove = new QAction(this);
	connect(aRemove, SIGNAL(triggered()), this, SLOT(actRemove()));
	
	messageList = new QListWidget;
	QToolBar *messageToolBar = new QToolBar;
	messageToolBar->setOrientation(Qt::Vertical);
	messageToolBar->addAction(aAdd);
	messageToolBar->addAction(aRemove);

	QSettings settings;
	settings.beginGroup("messages");
	int count = settings.value("count", 0).toInt();
	for (int i = 0; i < count; i++)
		messageList->addItem(settings.value(QString("msg%1").arg(i)).toString());
	
	QHBoxLayout *mainLayout = new QHBoxLayout;
	mainLayout->addWidget(messageList);
	mainLayout->addWidget(messageToolBar);

	setLayout(mainLayout);
	
	retranslateUi();
}

void MessagesSettingsPage::storeSettings()
{
	QSettings settings;
	settings.beginGroup("messages");
	settings.setValue("count", messageList->count());
	for (int i = 0; i < messageList->count(); i++)
		settings.setValue(QString("msg%1").arg(i), messageList->item(i)->text());
}

void MessagesSettingsPage::actAdd()
{
	bool ok;
	QString msg = QInputDialog::getText(this, tr("Add message"), tr("Message:"), QLineEdit::Normal, QString(), &ok);
	if (ok) {
		messageList->addItem(msg);
		storeSettings();
	}
}

void MessagesSettingsPage::actRemove()
{
	if (messageList->currentItem()) {
		delete messageList->takeItem(messageList->currentRow());
		storeSettings();
	}
}

void MessagesSettingsPage::retranslateUi()
{
	aAdd->setText(tr("&Add"));
	aRemove->setText(tr("&Remove"));
}

DlgSettings::DlgSettings(QWidget *parent)
	: QDialog(parent)
{
	connect(settingsCache, SIGNAL(langChanged()), this, SLOT(updateLanguage()));
	
	contentsWidget = new QListWidget;
	contentsWidget->setViewMode(QListView::IconMode);
	contentsWidget->setIconSize(QSize(96, 84));
	contentsWidget->setMovement(QListView::Static);
	contentsWidget->setMinimumWidth(115);
	contentsWidget->setMaximumWidth(130);
	contentsWidget->setSpacing(12);
	
	pagesWidget = new QStackedWidget;
	pagesWidget->addWidget(new GeneralSettingsPage);
	pagesWidget->addWidget(new AppearanceSettingsPage);
	pagesWidget->addWidget(new UserInterfaceSettingsPage);
	pagesWidget->addWidget(new MessagesSettingsPage);
	
	closeButton = new QPushButton;
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	createIcons();
	contentsWidget->setCurrentRow(0);
	
	QHBoxLayout *hboxLayout = new QHBoxLayout;
	hboxLayout->addWidget(contentsWidget);
	hboxLayout->addWidget(pagesWidget);
	
	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(closeButton);
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(hboxLayout);
	mainLayout->addSpacing(12);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);
	
	retranslateUi();
	
	resize(700, 450);
}

void DlgSettings::createIcons()
{
	generalButton = new QListWidgetItem(contentsWidget);
	generalButton->setTextAlignment(Qt::AlignHCenter);
	generalButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	generalButton->setIcon(QIcon(":/resources/icon_config_general.svg"));
	
	appearanceButton = new QListWidgetItem(contentsWidget);
	appearanceButton->setTextAlignment(Qt::AlignHCenter);
	appearanceButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	appearanceButton->setIcon(QIcon(":/resources/icon_config_appearance.svg"));
	
	userInterfaceButton = new QListWidgetItem(contentsWidget);
	userInterfaceButton->setTextAlignment(Qt::AlignHCenter);
	userInterfaceButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	userInterfaceButton->setIcon(QIcon(":/resources/icon_config_interface.svg"));
	
	messagesButton = new QListWidgetItem(contentsWidget);
	messagesButton->setTextAlignment(Qt::AlignHCenter);
	messagesButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	messagesButton->setIcon(QIcon(":/resources/icon_config_messages.svg"));
	
	connect(contentsWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem *)));
}

void DlgSettings::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	
	pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void DlgSettings::updateLanguage()
{
	qApp->removeTranslator(translator);
	installNewTranslator();
}

void DlgSettings::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
		retranslateUi();
	QDialog::changeEvent(event);
}

void DlgSettings::closeEvent(QCloseEvent *event)
{
	if (!db->getLoadSuccess())
		if (QMessageBox::critical(this, tr("Error"), tr("Your card database is invalid. Would you like to go back and set the correct path?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			event->ignore();
			return;
		}
	if (!QDir(settingsCache->getDeckPath()).exists())
		if (QMessageBox::critical(this, tr("Error"), tr("The path to your deck directory is invalid. Would you like to go back and set the correct path?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			event->ignore();
			return;
		}
	if (!QDir(settingsCache->getPicsPath()).exists())
		if (QMessageBox::critical(this, tr("Error"), tr("The path to your card pictures directory is invalid. Would you like to go back and set the correct path?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			event->ignore();
			return;
		}
	event->accept();
}

void DlgSettings::retranslateUi()
{
	setWindowTitle(tr("Settings"));
	
	generalButton->setText(tr("General"));
	appearanceButton->setText(tr("Appearance"));
	userInterfaceButton->setText(tr("User interface"));
	messagesButton->setText(tr("Messages"));
	
	closeButton->setText(tr("&Close"));
	
	for (int i = 0; i < pagesWidget->count(); i++)
		dynamic_cast<AbstractSettingsPage *>(pagesWidget->widget(i))->retranslateUi();
}