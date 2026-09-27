// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
// NoteDto.h  -- Unified Note -> RapidJSON serializer
//
// Purpose:
//   HandleListAll / HandleListTrash / HandleListArchived / HandleGetNote
//   each contained ~50 lines of near-identical AddMember code.
//   This module eliminates that duplication; any future field additions
//   only need to happen in one place.
//
// Deep-module design:
//   - Tiny interface (two static functions)
//   - Full RapidJSON detail hidden from callers
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include "rapidjson/document.h"
#include "core/domain/Note.h"
#include "core/domain/NoteItem.h"

// RapidJSON UTF-16 type aliases matching project convention
using RJDoc   = rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>>;
using RJValue = rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>;
using RJAlloc = rapidjson::MemoryPoolAllocator<>;

/// Maximum number of preview items in list responses
static constexpr size_t kNotePreviewLimit = 4;

class NoteDto
{
public:
    /// Serialize to a summary object (for all list views).
    /// Contains: 14 common header fields + itemCount/pendingCount/completedCount + previewItems
    static RJValue Summary(const Note& note, RJAlloc& alloc);

    /// Serialize to a detail object (for HandleGetNote / Export).
    /// Contains: 14 common header fields + full items array (no limit)
    static RJValue Detail(const Note& note, RJAlloc& alloc);

    /// Serialize a note's items array
    static RJValue SerializeItemsArray(const std::vector<NoteItem>& items, RJAlloc& alloc);

    /// Serialize a single NoteItem to { id, content, finished }
    static RJValue SerializeItem(const NoteItem& item, RJAlloc& alloc);

    /// Serialize note settings for sticker host window
    static RJValue SerializeSetting(const Note& note, RJAlloc& alloc);

    /// Deserialize Note from JSON Document
    static Note FromJson(const CString& name, const RJDoc& doc);

    /// Deserialize NoteItem from JSON Value
    static NoteItem ItemFromJson(const RJValue& v);

    /// Convert any RJValue into JSON CString
    static CString Stringify(const RJValue& val);

private:
    /// Build the shared 14-field header block
    static RJValue BuildHeader(const Note& note, RJAlloc& alloc);
};
