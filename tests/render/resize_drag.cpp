#include "resize_drag.hpp"

namespace render_tests
{
    namespace
    {
        // Пауза перед нажатием: наблюдатель кадров успевает открыть дублирование
        // A pause before pressing: the frame watcher manages to open the duplication
        constexpr DWORD PRESS_DELAY_MS = 300;

        // Событие мыши в абсолютных координатах всего рабочего стола (0…65535) / A mouse event in absolute coordinates of the whole desktop (0…65535)
        void sendMouse(POINT point, DWORD flags)
        {
            INPUT input{};
            int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
            int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
            int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            input.type = INPUT_MOUSE;
            input.mi.dx = static_cast<LONG>((point.x - left) * 65535LL / (width - 1));
            input.mi.dy = static_cast<LONG>((point.y - top) * 65535LL / (height - 1));
            input.mi.dwFlags = flags | MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            SendInput(1, &input, sizeof(input));
        }

        // Смещение шага в круге «вправо, затем обратно» (в шагах) / The step offset in the "right, then back" round (in steps)
        int stepOffset(int step, int steps) noexcept
        {
            if (step <= steps)
                return step;
            return 2 * steps - step;
        }

        // Тело потока мыши: нажать на углу, тянуть вправо-влево, отпустить (функтор вместо лямбды)
        // The mouse thread body: press at the corner, drag right and left, release (a functor instead of a lambda)
        struct DragDriver
        {
            std::shared_ptr<DragState> state;

            void operator()() const
            {
                const DragPlan& plan = state->plan;
                POINT point = plan.corner;

                Sleep(PRESS_DELAY_MS);
                sendMouse(point, 0);
                Sleep(50);
                sendMouse(point, MOUSEEVENTF_LEFTDOWN);
                for (int round = 0; round < plan.rounds; ++round)
                {
                    // Последний круг кончается вправо: окно остаётся выросшим / The last round ends to the right: the window stays grown
                    int last = 2 * plan.steps;
                    if (round == plan.rounds - 1)
                        last = plan.steps;
                    for (int step = 1; step <= last; ++step)
                    {
                        point.x = plan.corner.x + stepOffset(step, plan.steps) * plan.step;
                        sendMouse(point, 0);
                        Sleep(plan.gapMs);
                    }
                }
                sendMouse(point, MOUSEEVENTF_LEFTUP);
                state->done.store(true);
            }
        };
    }

    // Угол ищется в трёх пикселях от края рамки внутрь / The corner is looked for three pixels inside the frame edge
    bool resizeCorner(HWND window, POINT& corner)
    {
        RECT frame{};

        if (window == nullptr || !GetWindowRect(window, &frame))
            return false;
        corner.x = frame.right - 3;
        corner.y = frame.bottom - 3;
        LPARAM where = MAKELPARAM(static_cast<short>(corner.x), static_cast<short>(corner.y));
        return SendMessageW(window, WM_NCHITTEST, 0, where) == HTBOTTOMRIGHT;
    }

    void startDrag(const std::shared_ptr<DragState>& state)
    {
        state->thread = std::thread(DragDriver{state});
    }
}
