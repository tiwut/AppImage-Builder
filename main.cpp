#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QScrollBar>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QEventLoop>
#include <QFileInfo>

class AppImageBuilder : public QMainWindow {
    Q_OBJECT

public:
    AppImageBuilder() {
        setWindowTitle("Universal AppImage Builder");
        resize(850, 750);
        setupUI();
        applyTheme();
    }

private:
    QLineEdit *projectDirInput;
    QLineEdit *appNameInput;
    QLineEdit *exeNameInput;
    QLineEdit *iconInput;
    QLineEdit *categoriesInput;
    QPlainTextEdit *consoleOutput;
    QPushButton *btnBuild;

    void setupUI() {
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        QLabel *title = new QLabel("<b>📦 Universal AppImage Wizard</b>");
        title->setStyleSheet("font-size: 20px; color: #4ec9b0; margin-bottom: 5px;");
        mainLayout->addWidget(title);

        QGroupBox *step1 = new QGroupBox("Step 1: Source Project");
        QHBoxLayout *l1 = new QHBoxLayout(step1);
        projectDirInput = new QLineEdit();
        projectDirInput->setPlaceholderText("Select the folder containing CMakeLists.txt...");
        QPushButton *btnBrowseDir = new QPushButton("Browse...");
        connect(btnBrowseDir, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Project Folder");
            if (!dir.isEmpty()) projectDirInput->setText(dir);
        });
        l1->addWidget(projectDirInput);
        l1->addWidget(btnBrowseDir);
        mainLayout->addWidget(step1);

        QGroupBox *step2 = new QGroupBox("Step 2: Application Details");
        QFormLayout *l2 = new QFormLayout(step2);
        
        appNameInput = new QLineEdit();
        appNameInput->setPlaceholderText("e.g., Titan Store");
        
        exeNameInput = new QLineEdit();
        exeNameInput->setPlaceholderText("The exact target name in your CMakeLists (e.g., TitanStore)");

        QHBoxLayout *iconLayout = new QHBoxLayout();
        iconInput = new QLineEdit();
        iconInput->setPlaceholderText("Path to .png or .svg (Leave blank for default icon)");
        QPushButton *btnBrowseIcon = new QPushButton("Select Icon...");
        connect(btnBrowseIcon, &QPushButton::clicked, this, [this]() {
            QString file = QFileDialog::getOpenFileName(this, "Select App Icon", "", "Images (*.png *.svg)");
            if (!file.isEmpty()) iconInput->setText(file);
        });
        iconLayout->addWidget(iconInput);
        iconLayout->addWidget(btnBrowseIcon);

        categoriesInput = new QLineEdit("Development;Utility;");
        categoriesInput->setToolTip("Semicolon-separated categories for Linux app menus.");

        l2->addRow("<b>App Name:</b>", appNameInput);
        l2->addRow("<b>Executable Target:</b>", exeNameInput);
        l2->addRow("<b>Custom Icon:</b>", iconLayout);
        l2->addRow("<b>Categories:</b>", categoriesInput);
        mainLayout->addWidget(step2);

        connect(appNameInput, &QLineEdit::textChanged, this, [this](const QString &text) {
            if (exeNameInput->text().isEmpty() || exeNameInput->text() == text.left(text.length()-1).replace(" ", "")) {
                exeNameInput->setText(QString(text).replace(" ", ""));
            }
        });

        QGroupBox *step3 = new QGroupBox("Step 3: Build & Package");
        QVBoxLayout *l3 = new QVBoxLayout(step3);
        
        btnBuild = new QPushButton("🚀 Start Magic Build & Package");
        btnBuild->setMinimumHeight(45);
        btnBuild->setStyleSheet("background-color: #04395e; font-weight: bold; font-size: 15px;");
        connect(btnBuild, &QPushButton::clicked, this, &AppImageBuilder::startPackaging);
        l3->addWidget(btnBuild);

        consoleOutput = new QPlainTextEdit();
        consoleOutput->setReadOnly(true);
        QFont font("Monospace", 10);
        font.setStyleHint(QFont::Monospace);
        consoleOutput->setFont(font);
        consoleOutput->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; border: 1px solid #333;");
        l3->addWidget(consoleOutput);
        
        mainLayout->addWidget(step3);
        setCentralWidget(centralWidget);
    }

    void applyTheme() {
        qApp->setStyleSheet(R"(
            QMainWindow, QWidget { background-color: #252526; color: #cccccc; }
            QGroupBox { border: 1px solid #454545; border-radius: 5px; margin-top: 10px; font-weight: bold; padding-top: 15px; }
            QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #4ec9b0; }
            QLineEdit { background-color: #3c3c3c; border: 1px solid #555; padding: 6px; color: white; border-radius: 3px; }
            QPushButton { background-color: #333333; border: 1px solid #555; padding: 6px 15px; color: white; border-radius: 3px; }
            QPushButton:hover { background-color: #444444; }
        )");
    }

    void log(const QString &text) {
        consoleOutput->insertPlainText(text);
        consoleOutput->verticalScrollBar()->setValue(consoleOutput->verticalScrollBar()->maximum());
    }

    int runCommand(const QString &cmd, const QStringList &args, const QString &workDir) {
        QProcess proc;
        proc.setWorkingDirectory(workDir);

        QString fullCmd = cmd;
        for (const QString &arg : args) {
            QString safeArg = arg;
            safeArg.replace("'", "'\\''");
            fullCmd += " '" + safeArg + "'";
        }

        QString bashCmd = QString("env -u LD_LIBRARY_PATH -u APPDIR -u APPIMAGE APPIMAGE_EXTRACT_AND_RUN=1 %1").arg(fullCmd);

        proc.setProcessChannelMode(QProcess::MergedChannels);
        connect(&proc, &QProcess::readyReadStandardOutput, this, [&]() { 
            log(QString::fromUtf8(proc.readAllStandardOutput())); 
        });

        log("\n<b>> " + cmd + " " + args.join(" ") + "</b>\n");

        QEventLoop loop;
        connect(&proc, &QProcess::finished, &loop, &QEventLoop::quit);
        connect(&proc, &QProcess::errorOccurred, &loop, &QEventLoop::quit);

        proc.start("/bin/bash", {"-c", bashCmd});
        loop.exec(); 

        if (proc.error() == QProcess::FailedToStart) {
            log("❌ FAILED TO START COMMAND VIA BASH: " + cmd + "\n");
            return -1;
        }
        return proc.exitCode();
    }

    bool checkCMakeLists(const QString &projDir, const QString &exeName) {
        QFile file(projDir + "/CMakeLists.txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        QString content = file.readAll();
        file.close();

        QString expectedInstall = QString("install(TARGETS %1").arg(exeName);
        if (!content.contains(expectedInstall)) {
            QMessageBox::critical(this, "CMake Error Prevention", 
                "⚠️ Safety Check Failed!\n\n"
                "Your CMakeLists.txt is missing the install instruction for this executable.\n\n"
                "Please add this exact line to the bottom of your CMakeLists.txt:\n\n"
                "install(TARGETS " + exeName + " DESTINATION bin)");
            return false;
        }
        return true;
    }

    void startPackaging() {
        QString projDir = projectDirInput->text();
        QString appName = appNameInput->text();
        QString exeName = exeNameInput->text().trimmed().replace(" ", "_");
        QString customIcon = iconInput->text();
        QString categories = categoriesInput->text();

        if (projDir.isEmpty() || appName.isEmpty() || exeName.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please fill in Project Folder, App Name, and Executable Target.");
            return;
        }

        if (!QFile::exists(projDir + "/CMakeLists.txt")) {
            QMessageBox::warning(this, "Error", "CMakeLists.txt not found in the selected folder!");
            return;
        }

        if (!checkCMakeLists(projDir, exeName)) return;

        if (QStandardPaths::findExecutable("file").isEmpty()) {
            QMessageBox::critical(this, "Missing Dependency", "The 'file' command is missing.\nPlease open a terminal and run:\nsudo apt install file");
            return;
        }

        consoleOutput->clear();
        btnBuild->setEnabled(false);

        log("\n--- STEP 1: Checking linuxdeploy tools ---");
        if (!QFile::exists(projDir + "/linuxdeploy-x86_64.AppImage")) {
            log("\nDownloading linuxdeploy...");
            runCommand("wget", {"-O", "linuxdeploy-x86_64.AppImage", "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"}, projDir);
        }
        if (!QFile::exists(projDir + "/linuxdeploy-plugin-qt-x86_64.AppImage")) {
            log("\nDownloading Qt Plugin...");
            runCommand("wget", {"-O", "linuxdeploy-plugin-qt-x86_64.AppImage", "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"}, projDir);
        }

        runCommand("chmod", {"+x", "linuxdeploy-x86_64.AppImage"}, projDir);
        runCommand("chmod", {"+x", "linuxdeploy-plugin-qt-x86_64.AppImage"}, projDir);

        log("\n\n--- STEP 2: App Metadata & Icons ---");
        
        QString iconExt = "png";
        if (!customIcon.isEmpty() && QFile::exists(customIcon)) {
            log("\nCopying custom icon...");
            iconExt = QFileInfo(customIcon).suffix();
            QString targetIcon = projDir + "/" + exeName + "." + iconExt;
            if (customIcon != targetIcon) {
                QFile::remove(targetIcon); 
                QFile::copy(customIcon, targetIcon);
            }
        } else if (!QFile::exists(projDir + "/" + exeName + ".png")) {
            log("\nDownloading default icon...");
            runCommand("wget", {"-O", exeName + ".png", "https://raw.githubusercontent.com/microsoft/vscode/main/resources/linux/code.png"}, projDir);
        }

        log("\nGenerating .desktop file...");
        QFile file(projDir + "/" + exeName + ".desktop");
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\nType=Application\nName=" << appName 
                << "\nExec=" << exeName 
                << "\nIcon=" << exeName 
                << "\nCategories=" << categories << "\nTerminal=false\n";
            file.close();
        }

        log("\n\n--- STEP 3: Compiling Project ---");
        runCommand("rm", {"-rf", "build", "AppDir"}, projDir);
        QDir().mkpath(projDir + "/build");

        int res = runCommand("cmake", {"-DCMAKE_INSTALL_PREFIX=/usr", ".."}, projDir + "/build");
        if (res != 0) { log("\n❌ CMake Configuration Failed!\n"); btnBuild->setEnabled(true); return; }

        res = runCommand("make", {"-j" + QString::number(QThread::idealThreadCount())}, projDir + "/build");
        if (res != 0) { log("\n❌ Make Compilation Failed!\n"); btnBuild->setEnabled(true); return; }

        log("\n\n--- STEP 4: Installing to AppDir ---");
        res = runCommand("make", {"DESTDIR=../AppDir", "install"}, projDir + "/build");
        if (res != 0) { log("\n❌ Install Failed!\n"); btnBuild->setEnabled(true); return; }

        log("\n\n--- STEP 5: Generating AppImage ---");
        res = runCommand("./linuxdeploy-x86_64.AppImage", {
            "--appdir", "AppDir", 
            "--plugin", "qt", 
            "--output", "appimage", 
            "--desktop-file", exeName + ".desktop", 
            "--icon-file", exeName + "." + iconExt
        }, projDir);

        if (res == 0) {
            log("\n✅ SUCCESS! Your AppImage has been generated!\n");
            QMessageBox::information(this, "Success", "AppImage created successfully in:\n" + projDir);
        } else {
            log("\n❌ AppImage Generation Failed!\n");
        }

        btnBuild->setEnabled(true);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    AppImageBuilder builder;
    builder.show();
    return app.exec();
}

#include "main.moc"