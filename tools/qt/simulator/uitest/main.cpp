/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
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

#include <QApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

#include "graphic_config.h"
#include "main_widget.h"
#include "monitor.h"

extern void RunApp();
int main(int argc, char* argv[])
{
    QApplication uitest(argc, argv);
#ifdef _WIN32
    // 设置控制台输出编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 完全屏蔽Qt内部调试日志
    QLoggingCategory::setFilterRules(
        "qt.*=false\n"
        "*.debug=false\n"
        "*.info=true\n"
        "*.warning=true\n"
        "*.critical=true"
    );

    // 或者设置Qt日志级别为只显示警告和错误
    qSetMessagePattern("");

    std::cout << "=== OpenHarmony UI Lite Animation Test Console ===" << std::endl;
    std::cout << "Console output enabled for animation event callback logs" << std::endl;
    std::cout << "Program starting..." << std::endl;
#endif
    OHOS::GraphicStartUp::Init();
    OHOS::Monitor::GetInstance()->InitHal();
    OHOS::Monitor::GetInstance()->InitFontEngine();
    OHOS::Monitor::GetInstance()->InitImageDecodeAbility();

    if (argv[1] != nullptr && strcmp(argv[1], "-f") == 0) {
        OHOS::TcpSocketClientManager::GetInstance()->InitSocket();
    }

    RunApp();
    OHOS::MainWidget mainWidget;
    mainWidget.resize(HORIZONTAL_RESOLUTION, VERTICAL_RESOLUTION);
    mainWidget.show();
    return uitest.exec();
}
