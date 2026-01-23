#include "FanCurveEditorWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <algorithm>

FanCurveEditorWidget::FanCurveEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    m_points.clear();
    // initialize 10 evenly spaced points from 20..100 with linear duty 0..100
    const int count = 10;
    for (int i = 0; i < count; ++i) {
        double t = 20.0 + (80.0 * i) / (count - 1); // 20..100
        double d = (100.0 * i) / (count - 1); // 0..100
        m_points.append({t, d});
    }
    setMouseTracking(true);
}

void FanCurveEditorWidget::setPoints(const QVector<Point>& pts) {
    m_points = pts;
    sortPoints();
    update();
    emit pointsChanged(m_points);
}

void FanCurveEditorWidget::sortPoints() {
    std::sort(m_points.begin(), m_points.end(), [](const Point& a, const Point& b) { return a.temp < b.temp; });
}

QPointF FanCurveEditorWidget::toWidget(const Point& pt) const {
    // use same margins as paintEvent for consistent mapping
    const int left = 110, right = 20, top = 28, bottom = 68;
    double plotW = width() - left - right;
    double plotH = height() - top - bottom;
    double x = left + (pt.temp - 20.0) / 80.0 * plotW;
    double y = top + (1.0 - pt.duty / 100.0) * plotH;
    return QPointF(x, y);
}

FanCurveEditorWidget::Point FanCurveEditorWidget::fromWidget(const QPointF& pos) const {
    const int left = 110, right = 20, top = 28, bottom = 68;
    double plotW = width() - left - right;
    double plotH = height() - top - bottom;
    double temp = (pos.x() - left) / plotW * 80.0 + 20.0;
    double duty = (1.0 - (pos.y() - top) / plotH) * 100.0;
    return { std::clamp(temp, 20.0, 100.0), std::clamp(duty, 0.0, 100.0) };
}

QRectF FanCurveEditorWidget::pointRect(const Point& pt) const {
    QPointF c = toWidget(pt);
    return QRectF(c.x() - 7.0, c.y() - 7.0, 14.0, 14.0);
}

void FanCurveEditorWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#181e26"));
    // Margins for axes
    int left = 110, right = 20, top = 28, bottom = 68;
    QRectF plotRect(left, top, width() - left - right, height() - top - bottom);

    // Draw grid and ticks/labels
    QFont tickFont = font();
    tickFont.setPointSize(9);
    tickFont.setWeight(QFont::Normal);
    p.setFont(tickFont);
    QColor gridColor(50,50,50);
    QColor labelColor("#bdbdbd");

    // Y grid/ticks/labels (0-100% every 20%)
    for (int i = 0; i <= 5; ++i) {
        double frac = i / 5.0;
        qreal y = plotRect.top() + (1.0 - frac) * plotRect.height();
        qreal yy = qRound(y) + 0.5;
        // grid line
        p.setPen(gridColor);
        p.drawLine(QPointF(qRound(plotRect.left()) + 0.5, yy), QPointF(qRound(plotRect.right()) + 0.5, yy));
        // tick label
        int duty = i * 20;
        QString label = QString::number(duty) + "%";
        p.setPen(labelColor);
        QRectF labelRect(0, yy-12, left-16, 24);
        p.drawText(labelRect, Qt::AlignRight|Qt::AlignVCenter, label);
    }

    // X grid/ticks/labels (20-100°C every 10°C)
    for (int i = 0; i <= 8; ++i) {
        double frac = i / 8.0;
        qreal x = plotRect.left() + frac * plotRect.width();
        qreal xx = qRound(x) + 0.5;
        // grid line
        p.setPen(gridColor);
        p.drawLine(QPointF(xx, qRound(plotRect.top()) + 0.5), QPointF(xx, qRound(plotRect.bottom()) + 0.5));
        // tick label
        int temp = 20 + i * 10;
        QString label = QString::number(temp) + QChar(0x00B0) + "C";
        p.setPen(labelColor);
        QRectF labelRect(xx-20, plotRect.bottom()+12, 40, 20);
        p.drawText(labelRect, Qt::AlignHCenter|Qt::AlignTop, label);
    }

    // Axis labels
    QFont axisFont = font();
    axisFont.setPointSize(11);
    axisFont.setWeight(QFont::Normal);
    p.setPen(labelColor);
    // Y axis label rotated, outside tick labels
    QFont yFont = axisFont;
    yFont.setPointSize(10);
    p.save();
    int yLabelX = left/2 - 6; // put rotated label centered in the left margin, slight left offset
    p.translate(yLabelX, plotRect.center().y());
    p.rotate(-90);
    p.setFont(yFont);
    QRectF yLabelRect(-plotRect.height()/2, -12, plotRect.height(), 24);
    p.drawText(yLabelRect, Qt::AlignCenter, "% Duty");
    p.restore();
    // X axis label
    p.setFont(axisFont);
    QRectF xLabelRect(plotRect.left(), plotRect.bottom()+22, plotRect.width(), 20);
    p.drawText(xLabelRect, Qt::AlignHCenter|Qt::AlignTop, "Temperature (°C)");

    // Draw border around plot area (half-pixel aligned)
    p.setPen(QPen(labelColor, 1));
    QRectF borderRect(qRound(plotRect.left()) + 0.5, qRound(plotRect.top()) + 0.5,
                      qRound(plotRect.width()) - 1.0, qRound(plotRect.height()) - 1.0);
    p.drawRect(borderRect);

    // Draw curve
    p.setFont(tickFont);
    p.setPen(QPen(QColor("#3fa9f5"), 3));
    for (int i = 1; i < m_points.size(); ++i) {
        p.drawLine(toWidget(m_points[i-1]), toWidget(m_points[i]));
    }
    // Draw points
    for (int i = 0; i < m_points.size(); ++i) {
        QRectF r = pointRect(m_points[i]);
        p.setBrush(Qt::white);
        p.setPen(QPen(QColor("#3fa9f5"), 2));
        p.drawEllipse(r);
    }
}

void FanCurveEditorWidget::mousePressEvent(QMouseEvent* e) {
    for (int i = 0; i < m_points.size(); ++i) {
        if (pointRect(m_points[i]).contains(e->pos())) {
            m_draggedIndex = i;
            m_dragOffset = e->pos() - toWidget(m_points[i]).toPoint();
            return;
        }
    }
}

void FanCurveEditorWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_draggedIndex >= 0) {
        QPointF pos = e->pos() - m_dragOffset;
        Point pt = fromWidget(pos);
        // Clamp endpoints
        if (m_draggedIndex == 0) pt.temp = 20;
        if (m_draggedIndex == m_points.size()-1) pt.temp = 100;
        m_points[m_draggedIndex] = pt;
        sortPoints();
        update();
        emit pointsChanged(m_points);
    }
}

void FanCurveEditorWidget::mouseReleaseEvent(QMouseEvent*) {
    m_draggedIndex = -1;
}

void FanCurveEditorWidget::contextMenuEvent(QContextMenuEvent* e) {
    QMenu menu(this);
    QAction* add = menu.addAction("Add Point");
    QAction* rem = nullptr;
    int idx = -1;
    for (int i = 0; i < m_points.size(); ++i) {
        if (pointRect(m_points[i]).contains(e->pos())) {
            idx = i;
            if (m_points.size() > 2 && i != 0 && i != m_points.size()-1)
                rem = menu.addAction("Remove Point");
            break;
        }
    }
    QAction* chosen = menu.exec(e->globalPos());
    if (chosen == add) {
        Point pt = fromWidget(e->pos());
        addPoint(pt);
    } else if (rem && chosen == rem) {
        removePoint(idx);
    }
}

void FanCurveEditorWidget::addPoint(const Point& pt) {
    m_points.push_back(pt);
    sortPoints();
    update();
    emit pointsChanged(m_points);
}

void FanCurveEditorWidget::removePoint(int idx) {
    if (idx > 0 && idx < m_points.size()-1 && m_points.size() > 2) {
        m_points.remove(idx);
        update();
        emit pointsChanged(m_points);
    }
}
