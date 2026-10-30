#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/undo/undo_stack.hpp"
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
    void registerUndoStackTests(test::TestRunner& runner)
    {
        runner.add("undo: push one", testUndoStackPushOne);
        runner.add("undo: push after undo", testUndoStackPushAfterUndo);
        runner.add("undo: merge steps", testUndoStackMergeSteps);
        runner.add("undo: set clean", testUndoStackSetClean);
        runner.add("undo: limit", testUndoStackLimit);
        runner.add("undo: busy", testUndoStackBusy);
        runner.add("undo: observer", testUndoStackObserver);
        runner.add("undo: action step", testUndoStackActionStep);
        runner.add("undo: empty push", testUndoStackEmptyPush);
    }
}
