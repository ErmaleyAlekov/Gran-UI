#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/undo/undo_stack.hpp"
#include <stdexcept>

#include <limits>
#include <memory>

namespace
{
    class IntStep : public gran::undo::UndoStep
    {
    public:
        IntStep(int& value, int before, int after, int id)
            : _value(value), _before(before), _after(after), _id(id)
        {
            _value = _after;
        }

        void undo() override
        {
            _value = _before;
        }

        void redo() override
        {
            _value = _after;
        }

        int mergeId() const noexcept override
        {
            return _id;
        }

        bool mergeWith(const gran::undo::UndoStep& next) override
        {
            const IntStep& other = static_cast<const IntStep&>(next);
            _after = other._after;
            return true;
        }

    private:
        int& _value;
        int _before;
        int _after;
        int _id;
    };

    void testUndoStackPushOne()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        stack.push(std::make_unique<IntStep>(value, 0, 1, 1));
        TEST_ASSERT(stack.canUndo());
        TEST_ASSERT(!stack.canRedo());
        TEST_ASSERT(stack.count() == 1);
        TEST_ASSERT(stack.index() == 1);
        TEST_ASSERT(!stack.isClean());
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(value == 0);
        TEST_ASSERT(stack.canRedo());
        TEST_ASSERT(stack.isClean());
        TEST_ASSERT(stack.redo());
        TEST_ASSERT(value == 1);
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(!stack.undo());
    }

    void testUndoStackPushAfterUndo()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        stack.push(std::make_unique<IntStep>(value, 0, 1, 1));
        stack.undo();
        stack.push(std::make_unique<IntStep>(value, 1, 2, 2));
        TEST_ASSERT(stack.count() == 1);
        TEST_ASSERT(!stack.canRedo());
    }

    void testUndoStackMergeSteps()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        stack.push(std::make_unique<IntStep>(value, 0, 1, 1));
        stack.push(std::make_unique<IntStep>(value, 1, 2, 1));
        TEST_ASSERT(stack.count() == 1);
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(value == 0);
        TEST_ASSERT(stack.redo());
        TEST_ASSERT(value == 2);
        // После redo верхний шаг закрыт для объединения / After redo the top step is closed for merging
        stack.push(std::make_unique<IntStep>(value, 2, 3, 1));
        TEST_ASSERT(stack.count() == 2);
        stack.breakMerge();
        stack.push(std::make_unique<IntStep>(value, 3, 4, 1));
        TEST_ASSERT(stack.count() == 3);
        stack.push(std::make_unique<IntStep>(value, 4, 5, -1));
        stack.push(std::make_unique<IntStep>(value, 5, 6, -1));
        TEST_ASSERT(stack.count() == 5);
    }

    void testUndoStackSetClean()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        stack.push(std::make_unique<IntStep>(value, 0, 1, 1));
        stack.setClean();
        stack.push(std::make_unique<IntStep>(value, 1, 2, 1));
        TEST_ASSERT(stack.count() == 2);
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(stack.isClean());
    }

    void testUndoStackLimit()
    {
        int value = 0;
        gran::undo::UndoStack stack(2);
        stack.push(std::make_unique<IntStep>(value, 0, 1, -1));
        stack.push(std::make_unique<IntStep>(value, 1, 2, -1));
        stack.push(std::make_unique<IntStep>(value, 2, 3, -1));
        TEST_ASSERT(stack.count() == 2);
        TEST_ASSERT(stack.index() == 2);
        TEST_ASSERT(!stack.isClean());
        // «Чистая» позиция 0 ушла вместе с первым шагом / The clean position 0 went away with the first step
        stack.undo();
        stack.undo();
        TEST_ASSERT(value == 1);
        TEST_ASSERT(!stack.canUndo());
        TEST_ASSERT(!stack.isClean());
        stack.setLimit(1);
        TEST_ASSERT(stack.count() == 1);
    }

    // Максимальный предел сохраняет шаги по обе стороны курсора и отметку сохранения.
    // The maximum limit preserves steps on both sides of the cursor and the clean mark.
    void testUndoStackMaximumLimit()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        stack.push(std::make_unique<IntStep>(value, 0, 1, -1));
        stack.push(std::make_unique<IntStep>(value, 1, 2, -1));
        stack.setClean();
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(value == 1);
        stack.setLimit(std::numeric_limits<std::size_t>::max());
        TEST_ASSERT(stack.count() == 2);
        TEST_ASSERT(stack.canUndo());
        TEST_ASSERT(stack.canRedo());
        TEST_ASSERT(stack.redo());
        TEST_ASSERT(value == 2);
        TEST_ASSERT(stack.isClean());
    }

    // Шаг, который во время своей отмены пытается писать в стек и отменять снова / A step that tries to push and undo again during its own undo
    class BusyStep : public gran::undo::UndoStep
    {
    public:
        explicit BusyStep(gran::undo::UndoStack& stack) : _stack(stack) {}

        void undo() override
        {
            sawBusy = _stack.busy();
            _stack.push(std::make_unique<BusyStep>(_stack));
            nestedUndo = _stack.undo();
        }

        void redo() override {}

        bool sawBusy = false;
        bool nestedUndo = true;

    private:
        gran::undo::UndoStack& _stack;
    };

    void testUndoStackBusy()
    {
        gran::undo::UndoStack stack;
        std::unique_ptr<BusyStep> owned = std::make_unique<BusyStep>(stack);
        BusyStep* step = owned.get();

        stack.push(std::move(owned));
        TEST_ASSERT(!stack.busy());
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(step->sawBusy);
        TEST_ASSERT(!step->nestedUndo);
        TEST_ASSERT(stack.count() == 1);
        TEST_ASSERT(!stack.busy());
    }

    void testUndoStackObserver()
    {
        int value = 0;
        gran::undo::UndoStack stack;
        class CountObserver : public gran::undo::UndoObserver
        {
        public:
            void undoChanged(const gran::undo::UndoStack&) override
            {
                ++count;
            }

            int count = 0;
        };
        CountObserver observer;
        stack.setObserver(&observer);
        stack.push(std::make_unique<IntStep>(value, 0, 1, 1));
        stack.undo();
        stack.redo();
        stack.setClean();
        stack.clear();
        TEST_ASSERT(observer.count == 5);
    }

    void testUndoStackActionStep()
    {
        gran::undo::UndoStack stack;
        int value = 1;
        // Шаг кладётся уже выполненным: push не вызывает redo / The step is pushed already done: push does not call redo
        stack.push(std::make_unique<gran::undo::ActionStep>(
            [&value]() { value = 0; },
            [&value]() { value = 1; }
        ));
        TEST_ASSERT(value == 1);
        TEST_ASSERT(stack.undo());
        TEST_ASSERT(value == 0);
        TEST_ASSERT(stack.redo());
        TEST_ASSERT(value == 1);
        stack.clear();
        TEST_ASSERT(stack.count() == 0);
        TEST_ASSERT(stack.isClean());
    }

    void testUndoStackEmptyPush()
    {
        gran::undo::UndoStack stack;
        stack.push(nullptr);
        TEST_ASSERT(stack.count() == 0);
    }
}

namespace suites
{
    static void testCompensatedGroupRedo()
    {
        gran::undo::UndoStack stack;
        int value = 3;
        bool fail = false;
        stack.group([&] {
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { --value; }, [&] { ++value; }));
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { value -= 2; }, [&] {
                if (fail) throw std::runtime_error("redo");
                value += 2;
            }));
        });
        TEST_ASSERT(stack.undo() && value == 0);
        stack.setClean();
        fail = true;
        TEST_ASSERT_THROWS(stack.redo(), std::runtime_error);
        TEST_ASSERT(value == 0 && stack.index() == 0 && stack.isClean() && stack.canRedo() && !stack.canUndo());
        fail = false;
        TEST_ASSERT(stack.redo() && value == 3 && stack.index() == 1 && !stack.isClean());
        TEST_ASSERT(stack.undo() && value == 0 && stack.isClean());
    }

    static void verifyFailedGroupRedoCompensation(bool retryImmediately)
    {
        gran::undo::UndoStack stack(1);
        int value = 3;
        bool failUndo = false, failRedo = false;
        stack.group([&] {
            stack.push(std::make_unique<gran::undo::ActionStep>([&] {
                if (failUndo) throw std::runtime_error("compensation");
                --value;
            }, [&] { ++value; }));
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { value -= 2; }, [&] {
                if (failRedo) throw std::runtime_error("redo");
                value += 2;
            }));
        });
        stack.setClean();
        TEST_ASSERT(stack.undo() && value == 0);
        failUndo = failRedo = true;
        TEST_ASSERT_THROWS(stack.redo(), std::runtime_error);
        TEST_ASSERT(value == 1 && stack.index() == 0 && stack.count() == 1 && stack.canUndo() && stack.canRedo() && !stack.isClean());
        stack.setClean();
        TEST_ASSERT(stack.isClean());
        failUndo = failRedo = false;
        if (!retryImmediately)
            TEST_ASSERT(stack.undo() && value == 0 && !stack.isClean());
        TEST_ASSERT(stack.redo() && value == 3 && stack.index() == 1 && stack.count() == 1 && !stack.isClean());
        TEST_ASSERT(stack.undo() && value == 0 && !stack.isClean());
    }

    static void testFailedGroupRedoCompensation()
    {
        verifyFailedGroupRedoCompensation(true);
        verifyFailedGroupRedoCompensation(false);
    }

    static void testPartialGroupUndoNotifiesObserver()
    {
        struct Observer final : gran::undo::UndoObserver
        {
            void undoChanged(const gran::undo::UndoStack& stack) override { ++calls; clean = stack.isClean(); }
            int calls = 0;
            bool clean = false;
        } observer;
        gran::undo::UndoStack stack(1);
        int value = 7;
        bool failA = false, failB = false, failC = false;
        stack.group([&] {
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { if (failA) throw std::runtime_error("A"); --value; }, [&] { ++value; }));
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { if (failB) throw std::runtime_error("B"); value -= 2; }, [&] { value += 2; }));
            stack.push(std::make_unique<gran::undo::ActionStep>([&] { value -= 4; }, [&] { if (failC) throw std::runtime_error("C"); value += 4; }));
        });
        TEST_ASSERT(stack.undo() && value == 0);
        failB = failC = true;
        TEST_ASSERT_THROWS(stack.redo(), std::runtime_error);
        TEST_ASSERT(value == 3);
        stack.setObserver(&observer);
        stack.setClean();
        TEST_ASSERT(observer.clean);
        const int before = observer.calls;
        failA = true;
        failB = false;
        TEST_ASSERT_THROWS(stack.undo(), std::runtime_error);
        TEST_ASSERT(value == 1 && stack.index() == 0 && stack.canUndo() && stack.canRedo());
        TEST_ASSERT(observer.calls == before + 1 && !observer.clean && !stack.isClean());
    }

    void registerUndoStackTests(test::TestRunner& runner)
    {
        runner.add("undo: failed group redo compensates applied steps", testCompensatedGroupRedo);
        runner.add("undo: failed compensation preserves the prefix and suffix", testFailedGroupRedoCompensation);
        runner.add("undo: a partial group undo notifies the observer of dirty state", testPartialGroupUndoNotifiesObserver);
        runner.add("undo: push one", testUndoStackPushOne);
        runner.add("undo: push after undo", testUndoStackPushAfterUndo);
        runner.add("undo: merge steps", testUndoStackMergeSteps);
        runner.add("undo: set clean", testUndoStackSetClean);
        runner.add("undo: limit", testUndoStackLimit);
        runner.add("undo: maximum limit preserves redo and clean state", testUndoStackMaximumLimit);
        runner.add("undo: busy", testUndoStackBusy);
        runner.add("undo: observer", testUndoStackObserver);
        runner.add("undo: action step", testUndoStackActionStep);
        runner.add("undo: empty push", testUndoStackEmptyPush);
    }
}
