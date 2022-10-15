//
// Created by Kakawater on 2022/10/15.
//

#ifndef KAKAIMCLUSTER_CONCURRENTQUEUE_H
#define KAKAIMCLUSTER_CONCURRENTQUEUE_H

#include <memory>
#include <mutex>
#include <condition_variable>

namespace kakaIM {
    template<typename ElemType>
    class ConcurrentLinkedQueue {
    private:
        struct Node {
            std::shared_ptr<ElemType> data;
            std::unique_ptr<Node> next;
        };

        std::mutex head_mutex;
        std::unique_ptr<Node> head;
        std::mutex tail_mutex;
        Node *tail;
        std::condition_variable cond_vb;

        Node *get_tail() {
            std::lock_guard<std::mutex> lock(tail_mutex);
            return tail;
        }

        std::unique_ptr<Node> pop_head() {
            std::unique_ptr<Node> old_head = std::move(head);
            head = std::move(old_head->next);
            return old_head;
        }

        std::unique_lock<std::mutex> wait_for_data() {
            std::unique_lock<std::mutex> lock(head_mutex);
            cond_vb.wait(lock, [&] {
                return head != get_tail();
            });
            return std::move(lock);
        }

        std::unique_ptr<Node> wait_pop_head() {
            std::unique_lock<std::mutex> lock(wait_for_data());
            return pop_head();
        }

        std::unique_ptr<Node> wait_pop_head(ElemType &elem) {
            std::unique_lock<std::mutex> lock(wait_for_data());
            elem = std::move(*head->data);
            return pop_head();
        }

        std::unique_ptr<Node> try_pop_head() {
            std::lock_guard<std::mutex> lock(head_mutex);
            if (head.get() == get_tail()) {
                return nullptr;
            }
            return pop_head();
        }

        std::unique_ptr<Node> try_pop_head(ElemType &elem) {
            std::lock_guard<std::mutex> lock(head_mutex);
            if (head.get() == get_tail()) {
                return nullptr;
            }
            elem = std::move(*head->data);
            return pop_head();
        }

    public:
        ConcurrentLinkedQueue() :
                head(new Node), tail(head.get()) {
        }

        ConcurrentLinkedQueue(const ConcurrentLinkedQueue &other) = delete;

        ConcurrentLinkedQueue &operator=(const ConcurrentLinkedQueue &other) = delete;

        std::shared_ptr<ElemType> try_pop() {
            auto old_head = try_pop_head();
            if (old_head) {
                return old_head->data;
            } else {
                return nullptr;
            }
        }

        bool try_pop(ElemType &elem) {
            auto old_head = try_pop_head(elem);
            if (old_head) {
                elem = std::move(*old_head->data);
            }
            return old_head;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(head_mutex);
            return (head == get_tail());
        }

        std::shared_ptr<ElemType> wait_and_pop() {
            auto old_head = wait_pop_head();
            return old_head->data;
        }

        void wait_and_pop(ElemType &elem) {
            auto old_head = wait_pop_head(elem);
            if (old_head) {
                elem = std::move(*old_head->data);
            }
        }

        void push(ElemType elem) {
            auto data = std::make_shared<ElemType>(std::move(elem));
            std::unique_ptr<Node> node(new Node());
            {
                std::lock_guard<std::mutex> tail_lock(tail_mutex);
                tail->data = data;
                tail->next = std::move(node);
                tail = tail->next.get();
            }
            cond_vb.notify_one();
        }
    };

}

#endif //KAKAIMCLUSTER_CONCURRENTQUEUE_H
