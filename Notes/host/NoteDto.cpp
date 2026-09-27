// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/NoteDto.h"
#include "infra/AppSettingStore.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

RJValue NoteDto::SerializeItem(const NoteItem& item, RJAlloc& alloc)
{
    RJValue v(rapidjson::kObjectType);
    v.AddMember(rapidjson::StringRef(_T("id")),
                static_cast<uint64_t>(item.uId), alloc);
    v.AddMember(rapidjson::StringRef(_T("content")),
                RJValue(item.sContent.GetString(), alloc).Move(), alloc);
    v.AddMember(rapidjson::StringRef(_T("finished")),
                item.bFinished, alloc);
    v.AddMember(rapidjson::StringRef(_T("finish")),
                item.bFinished, alloc);
    return v;
}

RJValue NoteDto::BuildHeader(const Note& note, RJAlloc& alloc)
{
    RJValue vNote(rapidjson::kObjectType);

    // Identity
    vNote.AddMember(rapidjson::StringRef(_T("name")),
                    RJValue(note.name.GetString(), alloc).Move(), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("title")),
                    RJValue(note.title.GetString(), alloc).Move(), alloc);

    // Window rect
    RJValue vRect(rapidjson::kObjectType);
    vRect.AddMember(rapidjson::StringRef(_T("left")),   static_cast<int>(note.rect.left),   alloc);
    vRect.AddMember(rapidjson::StringRef(_T("top")),    static_cast<int>(note.rect.top),    alloc);
    vRect.AddMember(rapidjson::StringRef(_T("right")),  static_cast<int>(note.rect.right),  alloc);
    vRect.AddMember(rapidjson::StringRef(_T("bottom")), static_cast<int>(note.rect.bottom), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("rect")), vRect.Move(), alloc);

    // Appearance
    CString hexColor = note.ToHexColor();
    vNote.AddMember(rapidjson::StringRef(_T("bgColor")),
                    RJValue(hexColor.GetString(), alloc).Move(), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("opacity")),        note.opacity,        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("opacityEnabled")), note.opacityEnabled, alloc);
    vNote.AddMember(rapidjson::StringRef(_T("visible")),        note.visible,        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("topMost")),        note.topMost,        alloc);

    // State flags
    vNote.AddMember(rapidjson::StringRef(_T("isDeleted")),  note.isDeleted,                        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("deletedAt")),  static_cast<uint64_t>(note.deletedAt), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("isArchived")), note.isArchived,                       alloc);
    vNote.AddMember(rapidjson::StringRef(_T("archivedAt")), static_cast<uint64_t>(note.archivedAt),alloc);

    return vNote;
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

RJValue NoteDto::Summary(const Note& note, RJAlloc& alloc)
{
    RJValue vNote = BuildHeader(note, alloc);

    // Item statistics
    int pendingCount   = 0;
    int completedCount = 0;
    for (const auto& item : note.items)
    {
        if (item.bFinished) ++completedCount;
        else                ++pendingCount;
    }

    vNote.AddMember(rapidjson::StringRef(_T("itemCount")),
                    static_cast<int>(note.items.size()), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("pendingCount")),   pendingCount,   alloc);
    vNote.AddMember(rapidjson::StringRef(_T("completedCount")), completedCount, alloc);

    // Preview items (capped at kNotePreviewLimit)
    RJValue vPreview(rapidjson::kArrayType);
    const size_t previewCount = min(kNotePreviewLimit, note.items.size());
    for (size_t i = 0; i < previewCount; ++i)
    {
        vPreview.PushBack(SerializeItem(note.items[i], alloc).Move(), alloc);
    }
    vNote.AddMember(rapidjson::StringRef(_T("previewItems")), vPreview.Move(), alloc);

    return vNote;
}

RJValue NoteDto::Detail(const Note& note, RJAlloc& alloc)
{
    RJValue vNote = BuildHeader(note, alloc);
    vNote.AddMember(rapidjson::StringRef(_T("items")),
                    SerializeItemsArray(note.items, alloc).Move(), alloc);
    return vNote;
}

RJValue NoteDto::SerializeItemsArray(const std::vector<NoteItem>& items, RJAlloc& alloc)
{
    RJValue vItems(rapidjson::kArrayType);
    for (const auto& item : items)
    {
        vItems.PushBack(SerializeItem(item, alloc).Move(), alloc);
    }
    return vItems;
}

RJValue NoteDto::SerializeSetting(const Note& note, RJAlloc& alloc)
{
    RJValue vNote(rapidjson::kObjectType);

    vNote.AddMember(rapidjson::StringRef(_T("name")),
                    RJValue(note.name.GetString(), alloc).Move(), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("title")),
                    RJValue(note.title.GetString(), alloc).Move(), alloc);

    vNote.AddMember(rapidjson::StringRef(_T("left")),   static_cast<int>(note.rect.left),   alloc);
    vNote.AddMember(rapidjson::StringRef(_T("top")),    static_cast<int>(note.rect.top),    alloc);
    vNote.AddMember(rapidjson::StringRef(_T("right")),  static_cast<int>(note.rect.right),  alloc);
    vNote.AddMember(rapidjson::StringRef(_T("bottom")), static_cast<int>(note.rect.bottom), alloc);

    CString hexColor = note.ToHexColor();
    vNote.AddMember(rapidjson::StringRef(_T("bgcolor")),
                    RJValue(hexColor.GetString(), alloc).Move(), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("opacity")),        note.opacity,        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("opacityable")),    note.opacityEnabled, alloc);
    vNote.AddMember(rapidjson::StringRef(_T("visible")),        note.visible,        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("topmost")),        note.topMost,        alloc);

    vNote.AddMember(rapidjson::StringRef(_T("is_deleted")),  note.isDeleted,                        alloc);
    vNote.AddMember(rapidjson::StringRef(_T("deleted_at")),  static_cast<uint64_t>(note.deletedAt), alloc);
    vNote.AddMember(rapidjson::StringRef(_T("is_archived")), note.isArchived,                       alloc);
    vNote.AddMember(rapidjson::StringRef(_T("archived_at")), static_cast<uint64_t>(note.archivedAt),alloc);

    CString sLang = AppSettingStore::Load().sLanguage;
    if (sLang.IsEmpty()) sLang = _T("zh-CN");
    vNote.AddMember(rapidjson::StringRef(_T("language")),
                    RJValue(sLang.GetString(), alloc).Move(), alloc);

    return vNote;
}

NoteItem NoteDto::ItemFromJson(const RJValue& v)
{
    NoteItem item;
    if (!v.IsObject()) return item;

    if (v.HasMember(_T("id")))
    {
        const auto& vId = v[_T("id")];
        if (vId.IsUint64())      item.uId = vId.GetUint64();
        else if (vId.IsUint())   item.uId = static_cast<uint64_t>(vId.GetUint());
        else if (vId.IsInt64())  item.uId = static_cast<uint64_t>(vId.GetInt64());
        else if (vId.IsInt())    item.uId = static_cast<uint64_t>(vId.GetInt());
        else if (vId.IsDouble()) item.uId = static_cast<uint64_t>(vId.GetDouble());
        else if (vId.IsString()) item.uId = _wcstoui64(vId.GetString(), nullptr, 10);
    }

    if (v.HasMember(_T("content")) && v[_T("content")].IsString())
    {
        item.sContent = v[_T("content")].GetString();
    }

    if (v.HasMember(_T("finish")) && v[_T("finish")].IsBool())
    {
        item.bFinished = v[_T("finish")].GetBool();
    }
    else if (v.HasMember(_T("finished")) && v[_T("finished")].IsBool())
    {
        item.bFinished = v[_T("finished")].GetBool();
    }
    else if (v.HasMember(_T("finish")) && v[_T("finish")].IsInt())
    {
        item.bFinished = (v[_T("finish")].GetInt() != 0);
    }
    else if (v.HasMember(_T("finished")) && v[_T("finished")].IsInt())
    {
        item.bFinished = (v[_T("finished")].GetInt() != 0);
    }

    return item;
}

Note NoteDto::FromJson(const CString& name, const RJDoc& doc)
{
    Note note(name);

    if (doc.HasMember(_T("name")) && doc[_T("name")].IsString())
    {
        note.name = doc[_T("name")].GetString();
    }

    if (doc.HasMember(_T("title")) && doc[_T("title")].IsString())
    {
        note.title = doc[_T("title")].GetString();
    }

    int left   = (doc.HasMember(_T("left")) && doc[_T("left")].IsInt()) ? doc[_T("left")].GetInt() : 100;
    int top    = (doc.HasMember(_T("top")) && doc[_T("top")].IsInt()) ? doc[_T("top")].GetInt() : 100;
    int right  = (doc.HasMember(_T("right")) && doc[_T("right")].IsInt()) ? doc[_T("right")].GetInt() : 530;
    int bottom = (doc.HasMember(_T("bottom")) && doc[_T("bottom")].IsInt()) ? doc[_T("bottom")].GetInt() : 530;
    note.rect = CRect(left, top, right, bottom);

    if (doc.HasMember(_T("bgcolor")) && doc[_T("bgcolor")].IsString())
    {
        CString sColor = doc[_T("bgcolor")].GetString();
        sColor.TrimLeft(_T('#'));
        if (sColor.GetLength() == 6)
        {
            unsigned int r = 0, g = 0, b = 0;
            if (_stscanf_s(sColor.GetString(), _T("%02x%02x%02x"), &r, &g, &b) == 3)
            {
                note.bgColor = RGB(r & 0xFF, g & 0xFF, b & 0xFF);
            }
        }
    }
    else if (doc.HasMember(_T("bgcolor")) && doc[_T("bgcolor")].IsUint())
    {
        note.bgColor = static_cast<COLORREF>(doc[_T("bgcolor")].GetUint());
    }

    if (doc.HasMember(_T("opacity")) && doc[_T("opacity")].IsInt())
    {
        int op = doc[_T("opacity")].GetInt();
        if (op < 0) op = 0;
        else if (op > 100) op = 100;
        note.opacity = op;
    }

    if (doc.HasMember(_T("opacityable")) && doc[_T("opacityable")].IsBool())
    {
        note.opacityEnabled = doc[_T("opacityable")].GetBool();
    }
    else if (doc.HasMember(_T("opacityEnabled")) && doc[_T("opacityEnabled")].IsBool())
    {
        note.opacityEnabled = doc[_T("opacityEnabled")].GetBool();
    }

    if (doc.HasMember(_T("visible")) && doc[_T("visible")].IsBool())
    {
        note.visible = doc[_T("visible")].GetBool();
    }

    if (doc.HasMember(_T("topmost")) && doc[_T("topmost")].IsBool())
    {
        note.topMost = doc[_T("topmost")].GetBool();
    }
    else if (doc.HasMember(_T("topMost")) && doc[_T("topMost")].IsBool())
    {
        note.topMost = doc[_T("topMost")].GetBool();
    }

    if (doc.HasMember(_T("is_deleted")) && doc[_T("is_deleted")].IsBool())
    {
        note.isDeleted = doc[_T("is_deleted")].GetBool();
    }
    else if (doc.HasMember(_T("isDeleted")) && doc[_T("isDeleted")].IsBool())
    {
        note.isDeleted = doc[_T("isDeleted")].GetBool();
    }

    if (doc.HasMember(_T("deleted_at")) && doc[_T("deleted_at")].IsUint64())
    {
        note.deletedAt = doc[_T("deleted_at")].GetUint64();
    }
    else if (doc.HasMember(_T("deletedAt")) && doc[_T("deletedAt")].IsUint64())
    {
        note.deletedAt = doc[_T("deletedAt")].GetUint64();
    }

    if (doc.HasMember(_T("is_archived")) && doc[_T("is_archived")].IsBool())
    {
        note.isArchived = doc[_T("is_archived")].GetBool();
    }
    else if (doc.HasMember(_T("isArchived")) && doc[_T("isArchived")].IsBool())
    {
        note.isArchived = doc[_T("isArchived")].GetBool();
    }

    if (doc.HasMember(_T("archived_at")) && doc[_T("archived_at")].IsUint64())
    {
        note.archivedAt = doc[_T("archived_at")].GetUint64();
    }
    else if (doc.HasMember(_T("archivedAt")) && doc[_T("archivedAt")].IsUint64())
    {
        note.archivedAt = doc[_T("archivedAt")].GetUint64();
    }

    const RJValue* pItemsArray = nullptr;
    if (doc.HasMember(_T("items")) && doc[_T("items")].IsArray())
    {
        pItemsArray = &doc[_T("items")];
    }
    else if (doc.HasMember(_T("notes")) && doc[_T("notes")].IsArray())
    {
        pItemsArray = &doc[_T("notes")];
    }

    if (pItemsArray)
    {
        for (rapidjson::SizeType i = 0; i < pItemsArray->Size(); i++)
        {
            note.items.push_back(ItemFromJson((*pItemsArray)[i]));
        }
    }

    return note;
}

CString NoteDto::Stringify(const RJValue& val)
{
    rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
    rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
                      rapidjson::UTF16<TCHAR>,
                      rapidjson::UTF16<TCHAR>> writer(buffer);
    val.Accept(writer);
    return CString(buffer.GetString());
}
