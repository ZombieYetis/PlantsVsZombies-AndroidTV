/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PvZ/STL/list.h"

void pvzstl::detail::list_node_base::swap(list_node_base &x, list_node_base &y) noexcept {
    if (x.m_next != &x) {
        if (y.m_next != &y) {
            // Both x and y are not empty.
            std::swap(x.m_next, y.m_next);
            std::swap(x.m_prev, y.m_prev);
            x.m_next->m_prev = x.m_prev->m_next = &x;
            y.m_next->m_prev = y.m_prev->m_next = &y;
        } else {
            // x is not empty, y is empty.
            y.m_next = x.m_next;
            y.m_prev = x.m_prev;
            y.m_next->m_prev = y.m_prev->m_next = &y;
            x.m_next = x.m_prev = &x;
        }
    } else if (y.m_next != &y) {
        // x is empty, y is not empty.
        x.m_next = y.m_next;
        x.m_prev = y.m_prev;
        x.m_next->m_prev = x.m_prev->m_next = &x;
        y.m_next = y.m_prev = &y;
    }
}

void pvzstl::detail::list_node_base::transfer(list_node_base *first, list_node_base *last) noexcept {
    assert(first != last);

    if (this != last) {
        // Remove [first, last) from its old position.
        last->m_prev->m_next = this;
        first->m_prev->m_next = last;
        m_prev->m_next = first;

        // Splice [first, last) into its new position.
        list_node_base *const tmp = m_prev;
        m_prev = last->m_prev;
        last->m_prev = first->m_prev;
        first->m_prev = tmp;
    }
}

void pvzstl::detail::list_node_base::reverse() noexcept {
    list_node_base *tmp = this;
    do {
        std::swap(tmp->m_next, tmp->m_prev);

        // Old next node is now prev.
        tmp = tmp->m_prev;
    } while (tmp != this);
}

void pvzstl::detail::list_node_base::hook(list_node_base *pos) noexcept {
    m_next = pos;
    m_prev = pos->m_prev;
    pos->m_prev->m_next = this;
    pos->m_prev = this;
}

void pvzstl::detail::list_node_base::unhook() noexcept {
    list_node_base *const next_node = m_next;
    list_node_base *const prev_node = m_prev;
    prev_node->m_next = next_node;
    next_node->m_prev = prev_node;
}
