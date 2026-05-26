#ifndef RADIALGAUGE_H
#define RADIALGAUGE_H

#include <QWidget>

class RadialGauge : public QWidget {
    Q_OBJECT
public:
    explicit RadialGauge(QWidget* parent = nullptr);
    ~RadialGauge() = default;

    void setValue(double val);
    double value() const { return m_value; }

    void setLabel(const QString& label);
    QString label() const { return m_label; }

    void setSubLabel(const QString& subLabel);
    QString subLabel() const { return m_subLabel; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    double m_value = 0.0;
    QString m_label;
    QString m_subLabel;
};

#endif // RADIALGAUGE_H
