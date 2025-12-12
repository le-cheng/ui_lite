/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UI_TEST_TILE_VIEW_H
#define UI_TEST_TILE_VIEW_H

#include "components/ui_scroll_view.h"
#include "ui_test.h"
#include "components/ui_tile_view.h"

namespace OHOS {
class UITestTileView : public UITest {
public:
    UITestTileView() {}
    ~UITestTileView() {}
    void SetUp() override;
    void TearDown() override;
    const UIView* GetTestView() override;

    void UIKitTileViewTestDisplay001();

private:
    UIScrollView* container_ = nullptr;
    UITileView* tileView_ = nullptr;
};
} // namespace OHOS
#endif // UI_TEST_TILE_VIEW_H
