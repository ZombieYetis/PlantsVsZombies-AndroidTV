#pragma once

namespace netplay::snapshot {

// Optional menu widgets still occupy their fixed wire slots. Both peers must
// consume the same fields even when their resource files omit a widget.
template <typename Context, typename Value>
void SyncOptionalValue(Context &context, Value *value, Value absent = {}) {
    context.SyncBytes(value ? value : &absent, sizeof(Value));
}

template <typename Context, typename Widget>
void SyncWidget(Context &context, Widget *widget) {
    SyncOptionalValue(context, widget ? &widget->mX : nullptr);
    SyncOptionalValue(context, widget ? &widget->mY : nullptr);
    SyncOptionalValue(context, widget ? &widget->mWidth : nullptr);
    SyncOptionalValue(context, widget ? &widget->mHeight : nullptr);
    SyncOptionalValue(context, widget ? &widget->mVisible : nullptr);
    SyncOptionalValue(context, widget ? &widget->mMouseVisible : nullptr);
    SyncOptionalValue(context, widget ? &widget->mDisabled : nullptr, true);
}

} // namespace netplay::snapshot
