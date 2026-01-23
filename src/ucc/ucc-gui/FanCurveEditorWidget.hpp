#pragma once
#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPointF>

class FanCurveEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit FanCurveEditorWidget(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override { return QSize(400, 250); }
    QSize sizeHint() const override { return QSize(600, 350); }

    struct Point {
        double temp;
        double duty;
    };

    const QVector<Point>& points() const { return m_points; }
    void setPoints(const QVector<Point>& pts);

signals:
    void pointsChanged(const QVector<Point>&);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QVector<Point> m_points;
    int m_draggedIndex = -1;
    QPoint m_dragOffset;
    QRectF pointRect(const Point &pt) const;
    QPointF toWidget(const Point &pt) const;
    Point fromWidget(const QPointF &pos) const;
    void sortPoints();
    void addPoint(const Point &pt);
    void removePoint(int idx);
};
