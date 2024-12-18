#ifndef DRAGAREA_H
#define DRAGAREA_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QStringList>
#include <QVBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMouseEvent>
#include <QDebug>
#include <QFontMetrics>
#include <QScrollBar>

/**
 * @brief The DragArea class
 *
 * DragArea是一个自定义的控件，用于在GUI中显示文件拖拽和文件夹选择操作。
 * 它支持两种显示模式：
 * - FileSelectionMode：显示提示信息，可以拖拽文件或文件夹进来，并且点击可以打开文件夹选择对话框。
 * - LogOutputMode：显示日志信息，并在追加日志时自动滚动到底部。
 *
 * 用户可通过 setDisplayMode() 在两种模式之间切换。
 */
class DragArea : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 显示模式枚举
     */
    enum DisplayMode {
        FileSelectionMode, ///< 文件选择模式
        LogOutputMode      ///< 日志输出模式
    };

    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit DragArea(QWidget *parent = nullptr)
        : QWidget(parent),
        iconLabel(new QLabel(this)),
        textLabel(new QLabel(this)),
        scrollArea(new QScrollArea(this)),
        promptText("点击或将文件/文件夹拖拽至此"),
        filePath("./icons/file.png"),
        folderPath("./icons/folder.png"),
        customWindowTitle("拖放文件示例"),
        showingPrompt(true),
        singleFileMode(false),
        currentMode(FileSelectionMode)
    {
        setAcceptDrops(true);

        setAutoFillBackground(true);
        QPalette pal = this->palette();
        pal.setColor(QPalette::Window, QColor(240, 240, 240));
        setPalette(pal);

        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->hide();

        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        textLabel->setStyleSheet("color: #333;");
        textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许选择文本
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        textLabel->installEventFilter(this);        // 安装事件过滤器

        scrollArea->setWidget(textLabel);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(iconLabel, 0, Qt::AlignCenter);
        layout->addWidget(scrollArea, 1);

        setLayout(layout);
        setWindowTitle(customWindowTitle);
        resize(400, 300);

        // 初始显示提示
        showPrompt();
    }

    /**
     * @brief 设置显示模式
     * @param mode 要设置的显示模式
     */
    inline void setDisplayMode(DisplayMode mode)
    {
        currentMode = mode;
        if (mode == FileSelectionMode) {
            textLabel->setAlignment(Qt::AlignCenter);
            showPrompt();
            showingPrompt = true;
            singleFileMode = false;
            setAcceptDrops(true);
        } else if (mode == LogOutputMode) {
            textLabel->setAlignment(Qt::AlignLeft);
            iconLabel->hide();
            textLabel->clear();
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            showingPrompt = false;
            singleFileMode = false;
        }
        updateStyles();
    }

    /**
     * @brief 获取当前文件路径列表
     * @return 文件路径列表
     */
    inline QStringList getFilePaths() const
    {
        return filePaths;
    }

    /**
     * @brief 设置提示文本
     * @param str 提示文本
     */
    inline void setPromptText(const QString &str)
    {
        promptText = str;
        if (currentMode == FileSelectionMode && showingPrompt) {
            textLabel->setText(promptText);
        }
    }

    /**
     * @brief 设置文件图标路径
     * @param str 图标路径
     */
    inline void setFileIcon(const QString &str)
    {
        filePath = str;
    }

    /**
     * @brief 设置文件夹图标路径
     * @param str 图标路径
     */
    inline void setFolderIcon(const QString &str)
    {
        folderPath = str;
    }

    /**
     * @brief 设置窗口标题
     * @param str 标题文本
     */
    inline void setDragAreaTitle(const QString &str)
    {
        customWindowTitle = str;
        setWindowTitle(customWindowTitle);
    }

    /**
     * @brief 清空显示
     */
    inline void clearDisplay()
    {
        filePaths.clear();
        if (currentMode == FileSelectionMode) {
            showPrompt();
            showingPrompt = true;
            singleFileMode = false;
        } else if (currentMode == LogOutputMode) {
            textLabel->clear();
        }
        updateStyles();
    }

    /**
     * @brief 在日志模式下追加日志
     * @param log 要追加的日志信息
     */
    inline void appendLog(const QString &log)
    {
        if (currentMode != LogOutputMode)
            return;

        QString currentText = textLabel->text();
        if (!currentText.isEmpty()) {
            currentText += "\n";
        }
        currentText += log;
        textLabel->setText(currentText);

        QTimer::singleShot(0, this, [=]() {
            QScrollBar *vScroll = scrollArea->verticalScrollBar();
            vScroll->setValue(vScroll->maximum());
        });
    }

protected:
    /**
     * @brief 拖拽进入事件
     * @param event 拖拽事件指针
     */
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (currentMode != FileSelectionMode) {
            event->ignore();
            return;
        }

        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    }

    /**
     * @brief 放下文件事件
     * @param event 放下事件指针
     */
    void dropEvent(QDropEvent *event) override
    {
        if (currentMode != FileSelectionMode) {
            event->ignore();
            return;
        }

        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.isEmpty())
            return;

        filePaths.clear();
        QStringList paths;
        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                paths << url.toLocalFile();
            }
        }

        if (paths.isEmpty())
            return;

        filePaths = paths;
        QFileInfo info(paths.first());
        updateDisplay(paths, info.isDir());

        event->acceptProposedAction();
    }

    /**
     * @brief 鼠标点击事件
     * @param event 鼠标事件指针
     */
    void mousePressEvent(QMouseEvent *event) override
    {
        if (currentMode == FileSelectionMode && event->button() == Qt::LeftButton) {
            QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹");
            if (!dir.isEmpty()) {
                filePaths.clear();
                filePaths << dir;
                updateDisplay(filePaths, true);
            }
        } else {
            QWidget::mousePressEvent(event);
        }
    }

    /**
     * @brief 窗口尺寸变化事件
     * @param event 尺寸变化事件
     */
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateStyles();
    }

    /**
     * @brief 事件过滤器
     * @param obj 事件发生的对象
     * @param event 事件指针
     * @return 是否处理事件
     */
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == textLabel && currentMode == FileSelectionMode) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹");
                    if (!dir.isEmpty()) {
                        filePaths.clear();
                        filePaths << dir;
                        updateDisplay(filePaths, true);
                    }
                    return true; // 已处理该事件
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }

private:
    QLabel *iconLabel;     ///< 图标标签
    QLabel *textLabel;     ///< 文本显示标签
    QScrollArea *scrollArea;///< 滚动区域

    QString promptText;    ///< 提示文本
    QString filePath;      ///< 文件图标路径
    QString folderPath;    ///< 文件夹图标路径
    QString customWindowTitle; ///< 窗口标题

    QStringList filePaths; ///< 当前文件路径列表

    bool showingPrompt;    ///< 是否正在显示提示
    bool singleFileMode;   ///< 是否为单文件模式
    DisplayMode currentMode;///< 当前显示模式

    /**
     * @brief 显示提示
     */
    inline void showPrompt()
    {
        if (currentMode != FileSelectionMode)
            return;

        iconLabel->hide();
        iconLabel->clear();
        textLabel->setText(promptText);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        showingPrompt = true;
        singleFileMode = false;
        updateStyles();
    }

    /**
     * @brief 更新显示内容（文件列表）
     * @param paths 文件路径列表
     * @param isDir 是否为文件夹
     */
    inline void updateDisplay(const QStringList &paths, bool isDir)
    {
        if (currentMode != FileSelectionMode)
            return;

        showingPrompt = false;
        if (paths.size() == 1) {
            singleFileMode = true;
            if (isDir) {
                showFolderIcon(paths);
            } else {
                showFileIcon(paths);
            }
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else {
            singleFileMode = false;
            if (isDir) {
                showFolderIcon(paths);
            } else {
                showFileIcon(paths);
            }
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        updateStyles();
    }

    /**
     * @brief 显示文件图标
     * @param paths 文件路径列表
     */
    inline void showFileIcon(const QStringList &paths)
    {
        iconLabel->show();
        QPixmap pix(filePath);
        if (pix.isNull()) {
            qDebug() << "无法加载文件图标：" << filePath;
        }

        iconLabel->setPixmap(pix);

        if (paths.size() == 1) {
            applySingleFileStyle(paths.first());
        } else {
            applyMultiFileStyle(paths.join("\n"));
        }
    }

    /**
     * @brief 显示文件夹图标
     * @param paths 文件夹路径列表
     */
    inline void showFolderIcon(const QStringList &paths)
    {
        iconLabel->show();
        QPixmap pix(folderPath);
        if (pix.isNull()) {
            qDebug() << "无法加载文件夹图标：" << folderPath;
        }

        iconLabel->setPixmap(pix);

        if (paths.size() == 1) {
            applySingleFileStyle(paths.first());
        } else {
            applyMultiFileStyle(paths.join("\n"));
        }
    }

    /**
     * @brief 应用单文件样式
     * @param text 文件路径文本
     */
    inline void applySingleFileStyle(const QString &text)
    {
        QString displayText = insertZeroWidthSpaces(text, 10);
        textLabel->setText(displayText);
    }

    /**
     * @brief 应用多文件样式
     * @param text 多个文件路径文本（用换行分隔）
     */
    inline void applyMultiFileStyle(const QString &text)
    {
        textLabel->setText(text);
    }

    /**
     * @brief 更新样式（字体大小、图标大小等）
     */
    inline void updateStyles()
    {
        int w = width();
        if (w <= 0) w = 400;

        int promptFontSize = w / 12;      // 提示字体更大
        int singleFileFontSize = w / 25;  // 单文件模式字体
        int multiFileFontSize = w / 30;   // 多文件模式字体

        int largeIconSize = w / 3;        // 单文件大图标
        int smallIconSize = w / 6;        // 多文件小图标

        QFont f = textLabel->font();

        if (currentMode == FileSelectionMode) {
            if (showingPrompt) {
                f.setPointSize(promptFontSize);
                textLabel->setFont(f);
                iconLabel->hide();
                scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            } else {
                if (singleFileMode) {
                    f.setPointSize(singleFileFontSize);
                    textLabel->setFont(f);
                    QPixmap pix = iconLabel->pixmap(Qt::ReturnByValue);
                    if (!pix.isNull()) {
                        QPixmap scaled = pix.scaled(largeIconSize, largeIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        iconLabel->setPixmap(scaled);
                    }
                    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                } else {
                    f.setPointSize(multiFileFontSize);
                    textLabel->setFont(f);
                    QPixmap pix = iconLabel->pixmap(Qt::ReturnByValue);
                    if (!pix.isNull()) {
                        QPixmap scaled = pix.scaled(smallIconSize, smallIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        iconLabel->setPixmap(scaled);
                    }
                    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                }
            }
        } else if (currentMode == LogOutputMode) {
            int logFontSize = w / 30;
            f.setPointSize(logFontSize);
            textLabel->setFont(f);
            iconLabel->hide();
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }

        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    /**
     * @brief 每隔指定长度插入零宽空格的辅助函数
     * @param text 原始文本
     * @param maxSegmentLength 插入零宽空格的间隔
     * @return 处理后的文本
     */
    inline QString insertZeroWidthSpaces(const QString &text, int maxSegmentLength)
    {
        QString result;
        int count = 0;
        for (int i = 0; i < text.size(); ++i) {
            result.append(text[i]);
            count++;
            if (count >= maxSegmentLength) {
                result.append(QChar(0x200B));
                count = 0;
            }
        }
        return result;
    }
};

#endif // DRAGAREA_H
