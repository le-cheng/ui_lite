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

/**
 * 全局拖拽事件管理器 - 基本使用方法
 *
 * 1. 继承UIView和GlobalDragListener接口：
 *    class MyView : public UIView, public GlobalDragListener {
 *    public:
 *        MyView() {
 *            GlobalDragManager::GetInstance().RegisterListener(this, this, 10);
 *        }
 *
 *        ~MyView() {
 *            GlobalDragManager::GetInstance().UnregisterListener(this);
 *        }
 *
 *        bool OnGlobalDragStart(const DragEvent& event) override {
 *            StopMyAnimation();
 *            return false; // 继续传递事件
 *        }
 *
 *        bool OnGlobalDrag(const DragEvent& event) override {
 *            return false;
 *        }
 *
 *        bool OnGlobalDragEnd(const DragEvent& event) override {
 *            RestoreMyAnimation();
 *            return false;
 *        }
 *    };
 *
 * 2. 激活/停用监听器：
 *    GlobalDragManager::GetInstance().ActivateListener(myView);
 *    GlobalDragManager::GetInstance().DeactivateListener(myView);
 *
 * 注意事项：
 * - 优先级数值越大，优先级越高（先处理）
 * - 在构造函数中注册，析构函数中注销
 * - 返回false继续传递事件，返回true消费事件
 * - 支持运行时激活/停用监听器
 */

#include "global_drag_manager.h"
#include "gfx_utils/graphic_log.h"
#include <stdio.h>

namespace OHOS {

GlobalDragManager::GlobalDragManager()
{
    pthread_rwlock_init(&listenersMutex_, nullptr);
}

GlobalDragManager::~GlobalDragManager()
{
    pthread_rwlock_destroy(&listenersMutex_);
}

GlobalDragManager& GlobalDragManager::GetInstance()
{
    static GlobalDragManager instance;
    return instance;
}

bool GlobalDragManager::RegisterListener(UIView* view, GlobalDragListener* listener, int32_t priority)
{
    if (view == nullptr || listener == nullptr) {
        GRAPHIC_LOGE("GlobalDragManager::RegisterListener: Invalid parameters");
        return false;
    }

    // 检查是否已经注册
    GlobalDragListenerNode* existingNode = FindListenerNode(view);
    if (existingNode != nullptr) {
        GRAPHIC_LOGW("GlobalDragManager::RegisterListener: View already registered, updating listener");
        existingNode->listener = listener;
        existingNode->priority = priority;
        existingNode->isActive = true;
        return true;
    }

    // 创建新的监听器节点
    GlobalDragListenerNode* newNode = new GlobalDragListenerNode(view, listener, priority);
    if (newNode == nullptr) {
        GRAPHIC_LOGE("GlobalDragManager::RegisterListener: Failed to allocate memory for listener node");
        return false;
    }

    // 按优先级插入到链表中
    InsertListenerByPriority(newNode);

    GRAPHIC_LOGI("GlobalDragManager::RegisterListener: Successfully registered listener for view, priority=%d",
                 priority);

    return true;
}

bool GlobalDragManager::UnregisterListener(UIView* view)
{
    if (view == nullptr) {
        GRAPHIC_LOGE("GlobalDragManager::UnregisterListener: Invalid view parameter");
        return false;
    }



    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        if (node->data_->view == view) {
            GlobalDragListenerNode* listenerNode = node->data_;
            listeners_.Remove(node);
            delete listenerNode;
            GRAPHIC_LOGI("GlobalDragManager::UnregisterListener: Successfully unregistered listener for view");

            return true;
        }
        node = node->next_;
    }

    GRAPHIC_LOGW("GlobalDragManager::UnregisterListener: View not found in listener list");

    return false;
}

bool GlobalDragManager::ActivateListener(UIView* view)
{


    GlobalDragListenerNode* node = FindListenerNode(view);
    if (node == nullptr) {
        GRAPHIC_LOGW("GlobalDragManager::ActivateListener: View not found in listener list");

        return false;
    }

    node->isActive = true;
    GRAPHIC_LOGI("GlobalDragManager::ActivateListener: Successfully activated listener for view");

    return true;
}

bool GlobalDragManager::DeactivateListener(UIView* view)
{


    GlobalDragListenerNode* node = FindListenerNode(view);
    if (node == nullptr) {
        GRAPHIC_LOGW("GlobalDragManager::DeactivateListener: View not found in listener list");

        return false;
    }

    node->isActive = false;
    GRAPHIC_LOGI("GlobalDragManager::DeactivateListener: Successfully deactivated listener for view");

    return true;
}

void GlobalDragManager::DispatchGlobalDragStart(const DragEvent& event)
{


    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        GlobalDragListenerNode* listenerNode = node->data_;
        if (listenerNode->isActive && listenerNode->listener != nullptr) {
            try {
                if (listenerNode->listener->OnGlobalDragStart(event)) {
                    // 如果监听器消费了事件，可以选择是否继续传递给其他监听器
                    // 这里选择继续传递，让所有监听器都有机会处理
                    // printf("GlobalDragManager::DispatchGlobalDragStart: Event consumed by listener");
                }
            } catch (...) {
                GRAPHIC_LOGE("GlobalDragManager::DispatchGlobalDragStart: Exception in listener callback");
            }
        }
        node = node->next_;
    }

}

void GlobalDragManager::DispatchGlobalDrag(const DragEvent& event)
{
    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        GlobalDragListenerNode* listenerNode = node->data_;
        if (listenerNode->isActive && listenerNode->listener != nullptr) {
            if (listenerNode->listener->OnGlobalDrag(event)) {
                // printf("GlobalDragManager::DispatchGlobalDrag: Event consumed by listener");
            }
        }
        node = node->next_;
    }

}

void GlobalDragManager::DispatchGlobalDragEnd(const DragEvent& event)
{


    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        GlobalDragListenerNode* listenerNode = node->data_;
        if (listenerNode->isActive && listenerNode->listener != nullptr) {
            try {
                if (listenerNode->listener->OnGlobalDragEnd(event)) {
                    // printf("GlobalDragManager::DispatchGlobalDragEnd: Event consumed by listener");
                }
            } catch (...) {
                GRAPHIC_LOGE("GlobalDragManager::DispatchGlobalDragEnd: Exception in listener callback");
            }
        }
        node = node->next_;
    }

}

void GlobalDragManager::ClearAllListeners()
{


    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        GlobalDragListenerNode* listenerNode = node->data_;
        ListNode<GlobalDragListenerNode*>* nextNode = node->next_;
        listeners_.Remove(node);
        delete listenerNode;
        node = nextNode;
    }
    GRAPHIC_LOGI("GlobalDragManager::ClearAllListeners: All listeners cleared");

}

uint32_t GlobalDragManager::GetListenerCount() const
{
    pthread_rwlock_rdlock(const_cast<pthread_rwlock_t*>(&listenersMutex_));

    uint32_t count = 0;
    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        count++;
        node = node->next_;
    }
    pthread_rwlock_unlock(const_cast<pthread_rwlock_t*>(&listenersMutex_));
    return count;
}

GlobalDragListenerNode* GlobalDragManager::FindListenerNode(UIView* view)
{
    if (view == nullptr) {
        return nullptr;
    }

    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        if (node->data_->view == view) {
            return node->data_;
        }
        node = node->next_;
    }
    return nullptr;
}

void GlobalDragManager::InsertListenerByPriority(GlobalDragListenerNode* newNode)
{
    if (newNode == nullptr) {
        return;
    }

    if (listeners_.Size() == 0) {
        listeners_.PushBack(newNode);
        return;
    }

    ListNode<GlobalDragListenerNode*>* node = listeners_.Begin();
    while (node != listeners_.End()) {
        if (newNode->priority > node->data_->priority) {
            listeners_.Insert(node, newNode);
            return;
        }
        node = node->next_;
    }

    listeners_.PushBack(newNode);
}

} // namespace OHOS