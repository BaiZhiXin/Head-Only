#ifndef ANIMATEDPUSHBUTTON_H
#define ANIMATEDPUSHBUTTON_H

#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>

class AnimatedPushButton : public QPushButton
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化按钮的默认状态和动画效果
     *
     * @param parent 父窗口部件，默认为nullptr
     */
    explicit AnimatedPushButton(QWidget *parent = nullptr)
        : QPushButton(parent),
        hoverEnabled(true), clickEnabled(true), resetEnabled(true),
        isResetting(false),
        hoverDuration(150), clickDuration(100),
        resetDuration(100),
        hoverScaleFactor(1.3f),
        clickScaleFactor(1.0f),
        hoverEasingCurve(QEasingCurve::OutCubic),
        clickEasingCurve(QEasingCurve::OutQuad), resetEasingCurve(QEasingCurve::OutQuad)
    {
        setMouseTracking(true);  // 开启鼠标追踪
        setFocusPolicy(Qt::StrongFocus);

        // 初始化光环效果
        shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(15);  // 设置光环的模糊半径
        shadowEffect->setColor(Qt::blue);  // 设置光环颜色
        shadowEffect->setOffset(0, 0);  // 设置光环偏移量
        shadowEffect->setEnabled(false); // 初始时不显示光环
        setGraphicsEffect(shadowEffect); // 将效果应用到按钮
    }

    /**
     * @brief 设置鼠标悬停时的动画属性
     *
     * @param enabled 是否启用悬停动画，默认为true
     * @param duration 动画持续时间，单位为毫秒，默认为300ms
     * @param scaleFactor 悬停时的缩放比例，默认为1.2倍
     * @param easingCurve 悬停动画的缓动曲线，默认为OutCubic
     */
    void setHoverAnimation(bool enabled, int duration = 300, float scaleFactor = 1.2f, const QEasingCurve& easingCurve = QEasingCurve::OutCubic)
    {
        hoverEnabled = enabled;
        hoverDuration = duration;
        hoverScaleFactor = scaleFactor;
        hoverEasingCurve = easingCurve;
    }

    /**
     * @brief 设置点击时的动画属性
     *
     * @param enabled 是否启用点击动画，默认为true
     * @param duration 动画持续时间，单位为毫秒，默认为150ms
     * @param easingCurve 点击动画的缓动曲线，默认为OutQuad
     */
    void setClickAnimation(bool enabled, int duration = 150, const QEasingCurve& easingCurve = QEasingCurve::OutQuad)
    {
        clickEnabled = enabled;
        clickDuration = duration;
        clickEasingCurve = easingCurve;
    }

    /**
     * @brief 设置恢复到初始状态时的动画属性
     *
     * @param duration 动画持续时间，单位为毫秒，默认为200ms
     * @param easingCurve 恢复动画的缓动曲线，默认为OutQuad
     */
    void setResetAnimation(int duration = 200, const QEasingCurve& easingCurve = QEasingCurve::OutQuad)
    {
        resetEnabled = true;
        resetDuration = duration;
        resetEasingCurve = easingCurve;
    }

    /**
     * @brief 设置按钮的背景图片
     *
     * @param imagePath 背景图的文件路径
     */
    void setButtonImage(const QString &imagePath)
    {
        QPixmap pixmap(imagePath); // 加载图片
        if (pixmap.isNull()) {
            qDebug() << "Failed to load image at" << imagePath;
            return; // 如果加载失败，直接返回
        }

        currentBackgroundImagePath = imagePath; // 保存当前图像路径
        update(); // 触发重新绘制
    }

protected:
    /**
     * @brief 鼠标进入事件处理函数
     *
     * @param event 鼠标进入事件
     */
    void enterEvent(QEnterEvent *event) override
    {
        QPushButton::enterEvent(event);
        // 只有在没有动画时才获取按钮的原始几何信息
        if (!isResetting) {
            initialGeometry = geometry();
        }

        startHoverAnimation();
    }

    /**
     * @brief 鼠标离开事件处理函数
     *
     * @param event 鼠标离开事件
     */
    void leaveEvent(QEvent *event) override
    {
        QPushButton::leaveEvent(event);
        if (resetEnabled) {
            startResetAnimation();
        }
    }

    /**
     * @brief 鼠标按下事件处理函数
     *
     * @param event 鼠标按下事件
     */
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            QPushButton::mousePressEvent(event);  // 保持默认按键响应
            if (clickEnabled && !isResetting) {
                startClickAnimation();
            }
        }
    }

    /**
     * @brief 鼠标释放事件处理函数
     *
     * @param event 鼠标释放事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QPushButton::mouseReleaseEvent(event);
        if (clickEnabled && !isResetting) {
            startReleaseAnimation();
            // startResetAnimation();
        }
    }

    /**
     * @brief 绘制按钮时的自定义绘制函数
     *
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 如果当前按钮背景图路径不为空，绘制背景图
        if (!currentBackgroundImagePath.isEmpty()) {
            QPixmap pixmap(currentBackgroundImagePath);
            if (!pixmap.isNull()) {
                // 将图像绘制为按钮大小的背景图，保持图像比例
                QRect rect = this->rect(); // 获取按钮区域
                painter.drawPixmap(rect, pixmap);  // 将图像绘制为按钮大小的背景
            }
        }
    }

private:
    /**
     * @brief 启动鼠标悬停时的动画
     *
     * 该函数根据设置的动画参数，启动按钮的放大动画。
     */
    void startHoverAnimation()
    {
        if (isResetting) {
            return;  // 当前有动画在进行时，不启动新的动画
        }

        // 记录按钮的初始位置和大小
        QRect startRect = initialGeometry;
        QRect endRect = startRect.adjusted(
            -startRect.width() * (hoverScaleFactor - 1) / 2,
            -startRect.height() * (hoverScaleFactor - 1) / 2,
            startRect.width() * (hoverScaleFactor - 1) / 2,
            startRect.height() * (hoverScaleFactor - 1) / 2
            );

        hoverAnimation = new QPropertyAnimation(this, "geometry");
        hoverAnimation->setDuration(hoverDuration);
        hoverAnimation->setStartValue(startRect);
        hoverAnimation->setEndValue(endRect);
        hoverAnimation->setEasingCurve(hoverEasingCurve);
        hoverAnimation->start();
    }

    /**
     * @brief 启动恢复到初始状态的动画
     *
     * 该函数恢复按钮的尺寸，并关闭光环效果。
     */
    void startResetAnimation()
    {
        if (isResetting) {
            return;  // 当前有动画在进行时，不启动新的动画
        }

        isResetting = true;  // 设置恢复动画状态

        hoverAnimation->stop();
        // 恢复到初始状态
        resetAnimation = new QPropertyAnimation(this, "geometry");
        resetAnimation->setDuration(resetDuration);
        resetAnimation->setStartValue(geometry());
        resetAnimation->setEndValue(initialGeometry);  // 使用初始的几何值
        resetAnimation->setEasingCurve(resetEasingCurve);
        resetAnimation->start();

        // 恢复光环效果
        QPropertyAnimation *resetShadowEffect = new QPropertyAnimation(shadowEffect, "blurRadius");
        resetShadowEffect->setDuration(resetDuration);
        resetShadowEffect->setStartValue(30);
        resetShadowEffect->setEndValue(15);
        resetShadowEffect->start();

        // 在动画结束时关闭光环效果
        connect(resetShadowEffect, &QPropertyAnimation::finished, this, [this]() {
            shadowEffect->setEnabled(false);
            isResetting = false;  // 恢复动画结束，标记恢复状态结束
        });
    }

    /**
     * @brief 启动点击时的光环动画
     *
     * 该函数启动按钮的点击动画，并显示光环效果。
     */
    void startClickAnimation()
    {
        // 激活光环效果
        shadowEffect->setEnabled(true);  // 激活光环

        // 通过动画控制光环的大小
        clickAnimation = new QPropertyAnimation(shadowEffect, "blurRadius");
        clickAnimation->setDuration(clickDuration);
        clickAnimation->setStartValue(15);  // 初始光环模糊半径
        clickAnimation->setEndValue(40);   // 扩展光环的模糊半径
        clickAnimation->setEasingCurve(clickEasingCurve);
        clickAnimation->start();
    }

    /**
     * @brief 启动释放时的光环收回动画
     *
     * 该函数启动释放按钮后的光环收回动画。
     */
    void startReleaseAnimation()
    {
        // 通过动画控制光环的大小移除
        QPropertyAnimation *releaseAnimation = new QPropertyAnimation(shadowEffect, "blurRadius");
        releaseAnimation->setDuration(clickDuration);
        releaseAnimation->setStartValue(40);  // 当前光环模糊半径
        releaseAnimation->setEndValue(15);   // 恢复光环模糊半径
        releaseAnimation->setEasingCurve(QEasingCurve::OutQuad);
        releaseAnimation->start();

        // 动画完成后关闭光环
        connect(releaseAnimation, &QPropertyAnimation::finished, this, [this]() {
            shadowEffect->setEnabled(false);
        });
    }

private:
    bool hoverEnabled;               ///< 是否启用悬停动画
    bool clickEnabled;               ///< 是否启用点击动画
    bool resetEnabled;               ///< 是否启用恢复动画
    bool isResetting;                ///< 标记是否正在执行恢复动画

    int hoverDuration;               ///< 悬停动画持续时间
    int clickDuration;               ///< 点击动画持续时间
    int resetDuration;               ///< 恢复动画持续时间

    float hoverScaleFactor;          ///< 悬停时的缩放比例
    float clickScaleFactor;          ///< 点击时的缩放比例（不做缩放）

    QEasingCurve hoverEasingCurve;   ///< 悬停动画的缓动曲线
    QEasingCurve clickEasingCurve;   ///< 点击动画的缓动曲线
    QEasingCurve resetEasingCurve;   ///< 恢复动画的缓动曲线

    QRect initialGeometry;           ///< 按钮的初始几何信息（用于恢复动画）
    QString currentBackgroundImagePath;  ///< 当前按钮的背景图片路径
    QGraphicsDropShadowEffect *shadowEffect;  ///< 光环效果
    QPropertyAnimation *hoverAnimation = nullptr;  ///< 悬停动画
    QPropertyAnimation *clickAnimation = nullptr;  ///< 点击动画
    QPropertyAnimation *resetAnimation = nullptr;  ///< 恢复动画
};

#endif // ANIMATEDPUSHBUTTON_H
