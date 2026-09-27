// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <functional>
#include <vector>
#include <mutex>
#include <memory>

enum class NoteEventType {
	Created = 0,
	Updated,
	Deleted,
	VisibilityChanged,
	SettingsChanged
};

struct NoteEvent {
	NoteEventType type;
	CString       noteName;
	CString       extraJson;

	NoteEvent(NoteEventType t = NoteEventType::Updated, const CString& name = _T(""), const CString& extra = _T(""))
		: type(t), noteName(name), extraJson(extra) {}

	CString TypeToString() const {
		switch (type) {
		case NoteEventType::Created:           return _T("create");
		case NoteEventType::Updated:           return _T("update");
		case NoteEventType::Deleted:           return _T("delete");
		case NoteEventType::VisibilityChanged: return _T("visibility");
		case NoteEventType::SettingsChanged:   return _T("settings");
		default:                               return _T("update");
		}
	}
};

using NoteEventListener = std::function<void(const NoteEvent&)>;

class NoteEventBus {
public:
	static NoteEventBus& Instance() {
		static NoteEventBus s_bus;
		return s_bus;
	}

	int Subscribe(NoteEventListener listener) {
		std::lock_guard<std::mutex> lock(m_mutex);
		int id = ++m_nextId;
		m_listeners.push_back({ id, std::move(listener) });
		return id;
	}

	void Unsubscribe(int subscriptionId) {
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
			if (it->id == subscriptionId) {
				m_listeners.erase(it);
				break;
			}
		}
	}

	void Publish(const NoteEvent& event) {
		std::vector<NoteEventListener> targets;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (const auto& entry : m_listeners) {
				targets.push_back(entry.listener);
			}
		}
		for (const auto& fn : targets) {
			if (fn) {
				fn(event);
			}
		}
	}

	void Clear() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_listeners.clear();
	}

private:
	NoteEventBus() = default;
	~NoteEventBus() = default;
	NoteEventBus(const NoteEventBus&) = delete;
	NoteEventBus& operator=(const NoteEventBus&) = delete;

	struct ListenerEntry {
		int id;
		NoteEventListener listener;
	};

	std::mutex m_mutex;
	int m_nextId = 0;
	std::vector<ListenerEntry> m_listeners;
};
