// The About dialog (upstream's forms/About.dfm): the application, its
// version, the copyright of WinSCP it is based on, the links of the project,
// the translation and the third-party components; the license
// (forms/License.dfm).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <CoreMain.h>
#include <FtpFileSystem.h>
#include <PuttyTools.h>
#include <QtInterface.h>
#include <S3FileSystem.h>
#include <TextsCore.h>
#include <TextsWin.h>
#include <Translations.h>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const UnicodeString HomepageUrl(L"https://github.com/mikigal/openscp");
const UnicodeString IssuesUrl(L"https://github.com/mikigal/openscp/issues");
//---------------------------------------------------------------------------
QString Link(const UnicodeString & Url, const QString & Title = QString())
{
  QString AUrl = ToQString(Url).toHtmlEscaped();
  return QStringLiteral("<a href=\"%1\">%2</a>").arg(AUrl, Title.isEmpty() ? AUrl : Title.toHtmlEscaped());
}
//---------------------------------------------------------------------------
QString Para(const QStringList & Lines)
{
  return QStringLiteral("<p>%1</p>").arg(Lines.join(QStringLiteral("<br>")));
}
//---------------------------------------------------------------------------
QString Text(const UnicodeString & S)
{
  return ToQString(S).toHtmlEscaped();
}
//---------------------------------------------------------------------------
// The copyright of the translation in use (upstream's [Author] of
// translations/version.ini: the years and the translators)
QString TranslationCopyright(const UnicodeString & Code)
{
  QString Result;
  UnicodeString FileName = IncludeTrailingBackslash(TranslationsDirectory()) + L"version.ini";
  TTranslationFile File;
  try
  {
    ReadTranslationFile(FileName, File);
  }
  catch (Exception &)
  {
    return Result;
  }
  for (const auto & Entry : File[L"Author"])
  {
    if (SameText(Entry.first, Code))
    {
      QStringList Parts = ToQString(Entry.second).split(QLatin1Char(','));
      if (Parts.size() >= 2)
      {
        QString Years = (Parts[0] == Parts[1]) ? Parts[0] : QStringLiteral("%1–%2").arg(Parts[0], Parts[1]);
        Result = QStringLiteral("© %1 %2").arg(Years, Parts.mid(2).join(QStringLiteral(", "))).trimmed();
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
QString ThirdParty()
{
  QString Result;
  UnicodeString Translation = AppliedTranslation();
  if (!Translation.IsEmpty())
  {
    QLocale Locale(ToQString(TranslationLocale(Translation)).replace(QStringLiteral("@latin"), QStringLiteral("_Latn")));
    QString Language = Locale.nativeLanguageName();
    QString Header = ToQString(LoadStr(ABOUT_TRANSLATIONS_HEADER)).replace(QStringLiteral("{language}"), Language);
    Result += Para({ Header.toHtmlEscaped(), TranslationCopyright(Translation).toHtmlEscaped() });
  }
  Result += Para({ Text(LoadStr(ABOUT_THIRDPARTY_HEADER)) });
  Result += Para({
    Text(FMTLOAD(PUTTY_BASED_ON, (GetPuTTYVersion()))), Text(LoadStr(PUTTY_COPYRIGHT)),
    Link(LoadStr(PUTTY_LICENSE_URL), ToQString(LoadStr(ABOUT_THIRDPARTY_LICENSE))), Link(LoadStr(PUTTY_URL)) });
  // "OpenSSL 3.6.0 1 Oct 2025": the version and the year
  UnicodeString OpenSSLVersionText = GetOpenSSLVersionText();
  CutToChar(OpenSSLVersionText, L' ', true);
  UnicodeString OpenSSLVersion = CutToChar(OpenSSLVersionText, L' ', true);
  CutToChar(OpenSSLVersionText, L' ', true);
  CutToChar(OpenSSLVersionText, L' ', true);
  UnicodeString OpenSSLYear = CutToChar(OpenSSLVersionText, L' ', true);
  Result += Para({
    Text(FMTLOAD(OPENSSL_BASED_ON, (OpenSSLVersion))), Text(FMTLOAD(OPENSSL_COPYRIGHT2, (OpenSSLYear))),
    Link(LoadStr(OPENSSL_URL)) });
  Result += Para({ Text(LoadStr(FILEZILLA_BASED_ON2)), Text(LoadStr(FILEZILLA_COPYRIGHT2)), Link(LoadStr(FILEZILLA_URL)) });
  Result += Para({ Text(FMTLOAD(NEON_BASED_ON2, (NeonVersion()))), Text(LoadStr(NEON_COPYRIGHT)), Link(LoadStr(NEON_URL)) });
  Result += Para({
    Text(FMTLOAD(S3_BASED_ON, (S3LibVersion()))), Text(LoadStr(S3_COPYRIGHT)),
    Link(LoadStr(S3_LICENSE_URL), ToQString(LoadStr(ABOUT_THIRDPARTY_LICENSE))), Link(LoadStr(S3_URL)) });
  Result += Para({ Text(FMTLOAD(EXPAT_BASED_ON, (ExpatVersion()))), Link(LoadStr(EXPAT_URL)) });
  // In place of upstream's VCL components
  Result += Para({
    QStringLiteral("Qt %1").arg(QString::fromLatin1(qVersion())).toHtmlEscaped(),
    QStringLiteral("Copyright © The Qt Company Ltd. and other contributors").toHtmlEscaped(),
    Link(L"https://www.qt.io/") });
  return Result;
}
//---------------------------------------------------------------------------
// The license of the application (upstream's License dialog)
void DoLicenseDialog(QWidget * Parent)
{
  QDialog Dialog(Parent);
  Dialog.setObjectName(QStringLiteral("LicenseDialog"));
  Dialog.setWindowTitle(QDialog::tr("License"));
  QVBoxLayout * Layout = new QVBoxLayout(&Dialog);
  QPlainTextEdit * LicenseMemo = new QPlainTextEdit(&Dialog);
  LicenseMemo->setObjectName(QStringLiteral("LicenseMemo"));
  LicenseMemo->setReadOnly(true);
  QFile File(ApplicationDataFile(QStringLiteral("licenses/openscp/license.txt"), QStringLiteral("license.txt")));
  if (File.open(QIODevice::ReadOnly))
  {
    LicenseMemo->setPlainText(QString::fromUtf8(File.readAll()).replace(QStringLiteral("\r\n"), QStringLiteral("\n")));
  }
  Layout->addWidget(LicenseMemo);
  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Close, &Dialog);
  Buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("CloseButton"));
  QObject::connect(Buttons, &QDialogButtonBox::rejected, &Dialog, &QDialog::reject);
  Layout->addWidget(Buttons);
  Dialog.resize(Dialog.fontMetrics().averageCharWidth() * 90, Dialog.fontMetrics().height() * 30);
  Dialog.exec();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void DoAboutDialog()
{
  QDialog Dialog(QApplication::activeWindow());
  Dialog.setObjectName(QStringLiteral("AboutDialog"));
  Dialog.setWindowTitle(QDialog::tr("About application"));

  QVBoxLayout * MainLayout = new QVBoxLayout(&Dialog);
  QHBoxLayout * HeaderLayout = new QHBoxLayout();
  QLabel * IconPaintBox = new QLabel(&Dialog);
  IconPaintBox->setObjectName(QStringLiteral("IconPaintBox"));
  IconPaintBox->setPixmap(QApplication::windowIcon().pixmap(48, 48));
  HeaderLayout->addWidget(IconPaintBox, 0, Qt::AlignTop);
  QVBoxLayout * TextLayout = new QVBoxLayout();
  QLabel * ApplicationLabel = new QLabel(ToQString(AppName), &Dialog);
  ApplicationLabel->setObjectName(QStringLiteral("ApplicationLabel"));
  QFont Font = ApplicationLabel->font();
  Font.setBold(true);
  Font.setPointSizeF(Font.pointSizeF() * 1.4);
  ApplicationLabel->setFont(Font);
  TextLayout->addWidget(ApplicationLabel);
  QLabel * VersionLabel = new QLabel(ToQString(Configuration->VersionStr), &Dialog);
  VersionLabel->setObjectName(QStringLiteral("VersionLabel"));
  VersionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  TextLayout->addWidget(VersionLabel);
  // The port of WinSCP, its copyright
  QLabel * WinSCPCopyrightLabel = new QLabel(
    QStringLiteral("%1<br>%2").arg(QDialog::tr("A port of WinSCP to Linux").toHtmlEscaped(), Text(LoadStr(WINSCP_COPYRIGHT))), &Dialog);
  WinSCPCopyrightLabel->setObjectName(QStringLiteral("WinSCPCopyrightLabel"));
  TextLayout->addWidget(WinSCPCopyrightLabel);
  QLabel * ProductSpecificMessageLabel = new QLabel(QDialog::tr("To send comments and report bugs use support forum at:"), &Dialog);
  ProductSpecificMessageLabel->setObjectName(QStringLiteral("ProductSpecificMessageLabel"));
  ProductSpecificMessageLabel->setWordWrap(true);
  TextLayout->addWidget(ProductSpecificMessageLabel);
  for (const auto & Url : { std::make_pair("HomepageLabel", HomepageUrl), std::make_pair("ForumUrlLabel", IssuesUrl) })
  {
    QLabel * Label = new QLabel(Link(Url.second), &Dialog);
    Label->setObjectName(QString::fromLatin1(Url.first));
    Label->setOpenExternalLinks(true);
    TextLayout->addWidget(Label);
  }
  HeaderLayout->addLayout(TextLayout, 1);
  MainLayout->addLayout(HeaderLayout);

  QLabel * Label3 = new QLabel(QDialog::tr("Portions copyright:"), &Dialog);
  Label3->setObjectName(QStringLiteral("Label3"));
  MainLayout->addWidget(Label3);
  QTextBrowser * ThirdPartyPanel = new QTextBrowser(&Dialog);
  ThirdPartyPanel->setObjectName(QStringLiteral("ThirdPartyPanel"));
  ThirdPartyPanel->setOpenExternalLinks(true);
  ThirdPartyPanel->setHtml(ThirdParty());
  MainLayout->addWidget(ThirdPartyPanel, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &Dialog);
  Buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("OKButton"));
  QPushButton * LicenseButton = Buttons->addButton(QDialog::tr("&License..."), QDialogButtonBox::ActionRole);
  LicenseButton->setObjectName(QStringLiteral("LicenseButton"));
  QObject::connect(LicenseButton, &QPushButton::clicked, &Dialog, [&Dialog]() { DoLicenseDialog(&Dialog); });
  QObject::connect(Buttons, &QDialogButtonBox::accepted, &Dialog, &QDialog::accept);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"About.dfm"));
  Dialog.resize(Dialog.fontMetrics().averageCharWidth() * 70, Dialog.fontMetrics().height() * 32);
  Dialog.exec();
}
//---------------------------------------------------------------------------
void OpenHomepage()
{
  OpenBrowser(HomepageUrl);
}
//---------------------------------------------------------------------------
void OpenSupportForum()
{
  OpenBrowser(IssuesUrl);
}
//---------------------------------------------------------------------------
