#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QDateTime>
#include <QFontDatabase>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // Frameless + modal
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowModality(Qt::ApplicationModal);

    // Загрузка польззовательских шрифтов в базу шрифтов
    QFontDatabase::addApplicationFont(":/fonts/MoscowMetro.otf");
    QFontDatabase::addApplicationFont(":/fonts/PionerSans-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/PionerSans-Regular.ttf");

    setupFonts();
    setupTexts();

    // Привязка события нажатия кнопки закрытия окна
    connect(ui->pushButtonClose, &QPushButton::clicked,
            this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::setupFonts()
{
    // Подмена шрифта с сохранением остальных параметров файла дизайна
    auto setFamilyKeepUiSize = [](QWidget* w, const QString& family) {
        QFont f = w->font();
        f.setFamily(family);
        w->setFont(f);
    };

    setFamilyKeepUiSize(ui->labelFirstTitle,  "Moscow Metro");
    setFamilyKeepUiSize(ui->labelSecondTitle, "Moscow Metro");

    setFamilyKeepUiSize(ui->labelBuiltOn,    "Pioner Sans Medium");
    setFamilyKeepUiSize(ui->labelDeveloper,  "Pioner Sans Medium");
    setFamilyKeepUiSize(ui->labelGitHub,     "Pioner Sans Medium");
    setFamilyKeepUiSize(ui->labelEmail,      "Pioner Sans Medium");
    setFamilyKeepUiSize(ui->labelPoweredBy,  "Pioner Sans Medium");

    setFamilyKeepUiSize(ui->labelDeveloperVal, "Pioner Sans Regular");
    setFamilyKeepUiSize(ui->labelGitHubVal,    "Pioner Sans Regular");
    setFamilyKeepUiSize(ui->labelEmailVal,     "Pioner Sans Regular");

    // Установка цвета для всех надписей
    const QString black = "color: black;";
    for (auto *label : findChildren<QLabel*>())
        label->setStyleSheet(black);
}

static int monthIndex(const QString& m)
{
    static const QStringList months = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return months.indexOf(m) + 1;
}

static QString monthFullName(int m)
{
    static const QStringList full = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    if (m < 1 || m > 12) return QString();
    return full[m-1];
}

void AboutDialog::setupTexts()
{
    // Получение даты сборки программы
    const QString dateStr = QString(__DATE__).simplified();
    const QStringList parts = dateStr.split(' ');
    QString formatted = "Built on";

    if (parts.size() == 3) {
        const int m = monthIndex(parts[0]);
        const int d = parts[1].toInt();
        const int y = parts[2].toInt();

        // Получение времени сборки программы
        const QString timeStr = QString(__TIME__);
        const QStringList t = timeStr.split(':');
        const int hh = t.value(0).toInt();
        const int mm = t.value(1).toInt();

        formatted = QString("Built on %1 %2 %3, %4:%5")
                        .arg(monthFullName(m))
                        .arg(d)
                        .arg(y)
                        .arg(hh)
                        .arg(mm, 2, 10, QChar('0'));
    }

    ui->labelBuiltOn->setText(formatted);

    ui->labelDeveloperVal->setText("Ilia B. Anosov");

    ui->labelGitHubVal->setText(
        "<a href=\"https://github.com/lyceum-boy\" "
        "style=\"color:black; text-decoration:underline;\">"
        "lyceum-boy</a>"
    );
    ui->labelGitHubVal->setTextFormat(Qt::RichText);
    ui->labelGitHubVal->setOpenExternalLinks(true);

    ui->labelEmailVal->setText(
        "<a href=\"mailto:o737b04@voenmeh.ru\" "
        "style=\"color:black; text-decoration:underline;\">"
        "o737b04@voenmeh.ru</a>"
    );
    ui->labelEmailVal->setTextFormat(Qt::RichText);
    ui->labelEmailVal->setOpenExternalLinks(true);
}
