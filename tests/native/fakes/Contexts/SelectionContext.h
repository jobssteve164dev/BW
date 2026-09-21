#ifndef TEST_SELECTION_CONTEXT_H
#define TEST_SELECTION_CONTEXT_H

namespace contexts {

class SelectionContext {
public:
    static SelectionContext& getInstance() {
        static SelectionContext instance;
        return instance;
    }

    bool getIsLayoutSelected() const { return layoutSelected; }
    void setIsLayoutSelected(bool selected) { layoutSelected = selected; }

private:
    bool layoutSelected = false;
};

} // namespace contexts

#endif // TEST_SELECTION_CONTEXT_H
