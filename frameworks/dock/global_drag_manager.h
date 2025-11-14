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

#ifndef GRAPHIC_LITE_GLOBAL_DRAG_MANAGER_H
#define GRAPHIC_LITE_GLOBAL_DRAG_MANAGER_H

#include "components/ui_view.h"
#include "events/drag_event.h"
#include "gfx_utils/list.h"
#include <pthread.h>

namespace OHOS {

/**
 * @brief 全局drag事件监听器接口
 * 注册到全局drag事件链表的视图需要实现此接口
 */
class GlobalDragListener {
public:
    virtual ~GlobalDragListener() = default;

    /**
     * @brief 处理全局drag开始事件
     * @param event drag事件对象
     * @return true表示消费事件，false表示继续传递
     */
    virtual bool OnGlobalDragStart(const DragEvent& event) { return false; }

    /**
     * @brief 处理全局drag事件
     * @param event drag事件对象
     * @return true表示消费事件，false表示继续传递
     */
    virtual bool OnGlobalDrag(const DragEvent& event) { return false; }

    /**
     * @brief 处理全局drag结束事件
     * @param event drag事件对象
     * @return true表示消费事件，false表示继续传递
     */
    virtual bool OnGlobalDragEnd(const DragEvent& event) { return false; }
};

/**
 * @brief 全局drag事件注册项
 * 用于在链表中存储注册的视图和监听器信息
 */
struct GlobalDragListenerNode {
    UIView* view;                           // 注册的视图
    GlobalDragListener* listener;           // 监听器实例
    bool isActive;                          // 是否激活状态
    int32_t priority;                       // 优先级，数值越大优先级越高

    GlobalDragListenerNode() : view(nullptr), listener(nullptr), isActive(true), priority(0) {}
    GlobalDragListenerNode(UIView* v, GlobalDragListener* l, int32_t p = 0)
        : view(v), listener(l), isActive(true), priority(p) {}
};

/**
 * @brief 全局drag事件管理器
 * 维护一个全局的drag事件监听器链表，支持视图注册和注销
 * 当发生drag事件时，会通知所有注册的监听器
 */
class GlobalDragManager {
public:
    /**
     * @brief 获取全局drag事件管理器单例
     * @return 管理器实例引用
     */
    static GlobalDragManager& GetInstance();

    /**
     * @brief 注册全局drag事件监听器
     * @param view 要注册的视图
     * @param listener 监听器实例
     * @param priority 优先级，默认为0，数值越大优先级越高
     * @return true表示注册成功，false表示失败
     */
    bool RegisterListener(UIView* view, GlobalDragListener* listener, int32_t priority = 0);

    /**
     * @brief 注销全局drag事件监听器
     * @param view 要注销的视图
     * @return true表示注销成功，false表示未找到
     */
    bool UnregisterListener(UIView* view);

    /**
     * @brief 激活指定视图的监听器
     * @param view 要激活的视图
     * @return true表示操作成功，false表示未找到
     */
    bool ActivateListener(UIView* view);

    /**
     * @brief 停用指定视图的监听器
     * @param view 要停用的视图
     * @return true表示操作成功，false表示未找到
     */
    bool DeactivateListener(UIView* view);

    /**
     * @brief 分发全局drag开始事件
     * @param event drag事件对象
     */
    void DispatchGlobalDragStart(const DragEvent& event);

    /**
     * @brief 分发全局drag事件
     * @param event drag事件对象
     */
    void DispatchGlobalDrag(const DragEvent& event);

    /**
     * @brief 分发全局drag结束事件
     * @param event drag事件对象
     */
    void DispatchGlobalDragEnd(const DragEvent& event);

    /**
     * @brief 清空所有注册的监听器
     */
    void ClearAllListeners();

    /**
     * @brief 获取当前注册的监听器数量
     * @return 监听器数量
     */
    uint32_t GetListenerCount() const;

private:
    GlobalDragManager();
    ~GlobalDragManager();

    // 禁止拷贝和赋值
    GlobalDragManager(const GlobalDragManager&) = delete;
    GlobalDragManager& operator=(const GlobalDragManager&) = delete;

    /**
     * @brief 查找指定视图的监听器节点
     * @param view 要查找的视图
     * @return 找到的节点指针，未找到返回nullptr
     */
    GlobalDragListenerNode* FindListenerNode(UIView* view);

    /**
     * @brief 按优先级排序插入监听器节点
     * @param node 要插入的节点
     */
    void InsertListenerByPriority(GlobalDragListenerNode* node);

    List<GlobalDragListenerNode*> listeners_;   // 监听器链表
    mutable pthread_rwlock_t listenersMutex_;  // 监听器链表的读写锁
};

} // namespace OHOS

#endif // GRAPHIC_LITE_GLOBAL_DRAG_MANAGER_H