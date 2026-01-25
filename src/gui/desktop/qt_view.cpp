#include "qt_view.hpp"
#include <qt6/QtWidgets/QApplication>
#include <qt6/QtWidgets/QMainWindow>
#include <qt6/QtWidgets/QApplication>
#include <qt6/QtWidgets/QWidget>
#include <qt6/QtGui/QPainter>
#include <qt6/QtGui/QKeyEvent>
#include <queue>
#include <unordered_map>
#include <string>

// ---------- Внутренние структуры (скрыты за ViewHandle_t) ----------

struct Zone {
    int x, y, w, h;
    std::string name;
};

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {}

    // Запоминаем данные элементов по element_id
    void setElementData(const std::string &id, const ElementData_t &data) {
        elements_[id] = data;   // копируем ElementData_t
        update();               // запрос перерисовки
    }

    void setZones(const std::vector<Zone> &zones) {
        zones_ = zones;
    }

    // Очередь событий клавиатуры для poll_input()
    bool popInput(InputEvent_t &out) {
        if (inputQueue_.empty())
            return false;
        out = inputQueue_.front();
        inputQueue_.pop();
        return true;
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::black);

        // Рисуем зоны и их содержимое
        for (const Zone &z : zones_) {
            QRect rect(z.x, z.y, z.w, z.h);
            p.setPen(Qt::white);
            p.drawRect(rect);

            auto it = elements_.find(z.name);
            if (it == elements_.end())
                continue;

            const ElementData_t &data = it->second;

            switch (data.type) {
            case ELEMENT_TEXT: {
                p.drawText(rect.adjusted(4, 4, -4, -4),
                           Qt::AlignLeft | Qt::AlignTop,
                           QString::fromUtf8(data.content.text));
                break;
            }
            case ELEMENT_NUMBER: {
                QString s = QString::number(data.content.number);
                p.drawText(rect, Qt::AlignCenter, s);
                break;
            }
            case ELEMENT_MATRIX: {
                // Простая отрисовка матрицы квадратами
                int mw = data.content.matrix.width;
                int mh = data.content.matrix.height;
                const int *arr = data.content.matrix.data;

                if (mw <= 0 || mh <= 0 || !arr)
                    break;

                int cellW = rect.width() / mw;
                int cellH = rect.height() / mh;
                int cellSize = std::min(cellW, cellH);

                for (int row = 0; row < mh; ++row) {
                    for (int col = 0; col < mw; ++col) {
                        int value = arr[row * mw + col];
                        QRect cell(rect.x() + col * cellSize,
                                   rect.y() + row * cellSize,
                                   cellSize, cellSize);
                        if (value) {
                            p.fillRect(cell.adjusted(1, 1, -1, -1), getColorForValue(value));
                        } else {
                            p.setPen(Qt::gray);
                            p.drawRect(cell.adjusted(1, 1, -1, -1));
                        }
                    }
                }
                break;
            }
            }
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        InputEvent_t ev{};
        ev.key_state = 1;  // "нажата/удерживается"

        // Простое отображение Qt-ключей на "логические" коды, как в CLI[cite:203][cite:205]
        switch (event->key()) {
        case Qt::Key_Left:  ev.key_code = 'a'; break;
        case Qt::Key_Right: ev.key_code = 'd'; break;
        case Qt::Key_Up:    ev.key_code = 'w'; break;
        case Qt::Key_Down:  ev.key_code = 's'; break;
        default:
            ev.key_code = event->text().isEmpty()
                              ? 0
                              : event->text().at(0).toLatin1();
            break;
        }

        if (ev.key_code != 0)
            inputQueue_.push(ev);

        QWidget::keyPressEvent(event);
    }

private:
    std::vector<Zone> zones_;
    std::unordered_map<std::string, ElementData_t> elements_;
    std::queue<InputEvent_t> inputQueue_;
};

// Контекст Qt-View
struct QtViewContext {
    int width;
    int height;
    int fps;
    QMainWindow *window;
    GameWidget  *widget;
    std::vector<Zone> zones;
};

static QColor getColorForValue(int value) {
    switch (value) {
    case 1: return Qt::cyan;     // I
    case 2: return Qt::yellow;   // O
    case 3: return Qt::magenta;  // T
    case 4: return Qt::green;    // S
    case 5: return Qt::red;      // Z
    case 6: return Qt::blue;     // J
    case 7: return Qt::darkBlue; // L
    default: return Qt::white;
    }
}

// ---------- Реализация View_t для Qt ----------

static ViewHandle_t qt_init(int width, int height, int fps) {
    if (width <= 0 || height <= 0 || fps < 1)
        return nullptr;

    if (!QApplication::instance()) {
        return nullptr;
    }

    QtViewContext *ctx = new QtViewContext{};
    ctx->width = width;
    ctx->height = height;
    ctx->fps = fps;

    ctx->window = new QMainWindow;
    ctx->window->resize(800, 600);
    ctx->widget = new GameWidget;
    ctx->window->setCentralWidget(ctx->widget);

    // Показываем окно, но не блокируем
    if (!ctx->window->isVisible()) {
        ctx->window->show();
        QApplication::processEvents();
    }

    return static_cast<ViewHandle_t>(ctx);
}

static ViewResult_t qt_configure_zone(ViewHandle_t handle,
                                      const char *element_id,
                                      int x, int y, int max_w, int max_h)
{
    if (!handle) return VIEW_NOT_INITIALIZED;
    if (!element_id || strlen(element_id) == 0) return VIEW_BAD_DATA;
    if (x < 0 || y < 0 || max_w <= 0 || max_h <= 0) return VIEW_BAD_DATA;

    QtViewContext *ctx = static_cast<QtViewContext*>(handle);
    Zone z;
    z.x = x;
    z.y = y;
    z.w = max_w;
    z.h = max_h;
    z.name = element_id;

    ctx->zones.push_back(z);
    ctx->widget->setZones(ctx->zones);

    return VIEW_OK;
}

static ViewResult_t qt_draw_element(ViewHandle_t handle,
                                    const char *element_id,
                                    const ElementData_t *data)
{
    if (!handle) return VIEW_NOT_INITIALIZED;
    if (!element_id || !data) return VIEW_BAD_DATA;

    QtViewContext *ctx = static_cast<QtViewContext*>(handle);
    ctx->widget->setElementData(element_id, *data);

    return VIEW_OK;
}

static ViewResult_t qt_render(ViewHandle_t handle) {
    if (!handle) return VIEW_NOT_INITIALIZED;

    QtViewContext *ctx = static_cast<QtViewContext*>(handle);
    ctx->widget->update();  // явно запрашиваем перерисовку

    return VIEW_OK;
}

static ViewResult_t qt_poll_input(ViewHandle_t handle, InputEvent_t *event) {
    if (!handle) return VIEW_NOT_INITIALIZED;
    if (!event) return VIEW_ERROR;

    QtViewContext *ctx = static_cast<QtViewContext*>(handle);
    InputEvent_t ev{};
    if (ctx->widget->popInput(ev)) {
        *event = ev;
        return VIEW_OK;
    }

    return VIEW_NO_EVENT;
}
static ViewResult_t qt_shutdown(ViewHandle_t handle) {
    if (!handle) return VIEW_NOT_INITIALIZED;

    QtViewContext *ctx = static_cast<QtViewContext*>(handle);
    if (ctx->window) {
        ctx->window->close();
        delete ctx->window;
    }
    delete ctx;

    return VIEW_OK;
}

// Экспортируемый экземпляр Qt-View
const ViewInterface qt_view = {
    .version = VIEW_INTERFACE_VERSION,
    .init            = qt_init,
    .configure_zone  = qt_configure_zone,
    .draw_element    = qt_draw_element,
    .render          = qt_render,
    .poll_input      = qt_poll_input,
    .shutdown        = qt_shutdown
};