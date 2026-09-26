// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Testing & Reliability Suite (2026)
// Comprehensive Unit & Reliability Tests
// ------------------------------------------------------------------
#include <afxwin.h>
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>

using std::vector;

#include "core/domain/Note.h"
#include "core/domain/NoteItem.h"
#include "core/domain/AppSetting.h"
#include "core/services/NoteService.h"
#include "infra/JsonNoteRepository.h"
#include "infra/SqliteNoteRepository.h"
#include "infra/SqliteMigrator.h"
#include "infra/AppSettingStore.h"
#include "infra/DataMigrator.h"
#include "host/WebMessageDispatcher.h"
#include "host/NoteHostProtocol.h"
#include "host/NoteManagerProtocol.h"
#include "host/NoteDto.h"
#include "core/services/NoteEventBus.h"
#include "ref/Path.h"
#include "ref/XFile.h"
#include "ref/Log.h"
#include "ref/sqlite/sqlite3.h"

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (expr) { \
            std::wcout << L"  [PASS] " << msg << std::endl; \
            g_passCount++; \
        } else { \
            std::wcout << L"  [FAIL] " << msg << L" (" << _T(#expr) << L")" << std::endl; \
            g_failCount++; \
        } \
    } while(0)

void Test_64BitTimestamp_And_Persistence()
{
    std::wcout << L"\n[TEST SUITE 1] 64-bit ID Boundary & Persistence Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage\\"));
    Easy::Path::Create(testDir);
    JsonNoteRepository repo(testDir);
    NoteService service(repo);

    CString noteName = _T("test_64bit_note");
    Note note(noteName);
    note.title = _T("64-Bit Test Note");

    uint64_t id1 = 1724063216000ULL; // Standard JS millisecond timestamp
    uint64_t id2 = 1724063216999123ULL; // Microsecond precision compound ID
    uint64_t id3 = 0xFFFFFFFFFFFFFFFEULL; // Large uint64 boundary

    service.AddItem(note, NoteItem(id1, _T("Task 1 with JS timestamp"), false));
    service.AddItem(note, NoteItem(id2, _T("Task 2 with Micro timestamp"), true));
    service.AddItem(note, NoteItem(id3, _T("Task 3 with Uint64 boundary"), false));

    // Reload from disk
    Note loadedNote;
    bool loadOk = repo.Load(noteName, loadedNote);
    TEST_ASSERT(loadOk, L"Load persisted note from disk");
    TEST_ASSERT(loadedNote.items.size() == 3, L"Note items count matches exactly");

    if (loadedNote.items.size() == 3) {
        TEST_ASSERT(loadedNote.items[0].uId == id1, L"Item 1: 64-bit JS timestamp preserved without 32-bit truncation");
        TEST_ASSERT(loadedNote.items[1].uId == id2, L"Item 2: Compound timestamp preserved");
        TEST_ASSERT(loadedNote.items[2].uId == id3, L"Item 3: Large uint64 boundary preserved");
        TEST_ASSERT(loadedNote.items[1].bFinished == true, L"Item 2: Finished state preserved");
    }

    // Cleanup
    repo.Delete(noteName);
}

void Test_SpecialCharacters_And_Unicode()
{
    std::wcout << L"\n[TEST SUITE 2] Special Characters, Unicode & Emoji Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage\\"));
    JsonNoteRepository repo(testDir);
    NoteService service(repo);

    CString noteName = _T("test_unicode_note");
    Note note(noteName);
    note.title = _T("🎉 特殊字符测试 \"Quotes\" & \\Backslashes\\");

    CString content1 = _T("Hello \"World\"! \nLine 2 with \t tabs and \\ backslashes.");
    CString content2 = _T("中文测试：便签待办事项 ⚡ 🚀 🎈");
    CString content3 = _T("Special markdown: `code block` *italic* **bold** [link](https://test.com)");

    service.AddItem(note, NoteItem(1001, content1, false));
    service.AddItem(note, NoteItem(1002, content2, false));
    service.AddItem(note, NoteItem(1003, content3, false));

    Note loadedNote;
    bool loadOk = repo.Load(noteName, loadedNote);
    TEST_ASSERT(loadOk, L"Load note containing Unicode and special characters");
    if (loadedNote.items.size() == 3) {
        TEST_ASSERT(loadedNote.title == note.title, L"Title with Unicode & quotes preserved");
        TEST_ASSERT(loadedNote.items[0].sContent == content1, L"Content with quotes, newlines, tabs, backslashes preserved");
        TEST_ASSERT(loadedNote.items[1].sContent == content2, L"CJK characters and Emojis preserved");
        TEST_ASSERT(loadedNote.items[2].sContent == content3, L"Markdown markup preserved");
    }

    repo.Delete(noteName);
}

void Test_CorruptedJson_Resilience()
{
    std::wcout << L"\n[TEST SUITE 3] Corrupted & Malformed JSON Resilience Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage\\"));
    JsonNoteRepository repo(testDir);

    // Case 1: Corrupted JSON syntax
    CString corruptFile = Easy::Path::Resolve(testDir, _T("corrupt_syntax.json"));
    Easy::XFile::WriteFile(corruptFile, _T("{ \"name\": \"broken\", \"items\": [ { incomplete..."));

    Note outNote1;
    bool res1 = repo.Load(_T("corrupt_syntax"), outNote1);
    TEST_ASSERT(!res1, L"Gracefully handles corrupt JSON syntax without crashing");

    // Case 2: Wrong types in JSON
    CString wrongTypeFile = Easy::Path::Resolve(testDir, _T("wrong_type.json"));
    Easy::XFile::WriteFile(wrongTypeFile, _T("{ \"name\": \"wrong\", \"left\": \"not_a_number\", \"notes\": \"should_be_array\" }"));

    Note outNote2;
    bool res2 = repo.Load(_T("wrong_type"), outNote2);
    TEST_ASSERT(res2, L"Gracefully parses wrong types using default values");
    TEST_ASSERT(outNote2.rect.left == 100, L"Fallback to default coordinate when type mismatch occurs");

    // Case 3: Non-existent file
    Note outNote3;
    bool res3 = repo.Load(_T("non_existent_file_99999"), outNote3);
    TEST_ASSERT(!res3, L"Gracefully returns false for non-existent note file");

    DeleteFile(corruptFile);
    DeleteFile(wrongTypeFile);
}

void Test_Service_BusinessRules()
{
    std::wcout << L"\n[TEST SUITE 4] Service Layer Business Rules & Deduplication..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage\\"));
    JsonNoteRepository repo(testDir);
    NoteService service(repo);

    CString noteName = _T("test_service_note");
    Note note(noteName);

    // Test 1: ID = 0 auto-assignment
    NoteItem itemZero(0, _T("Zero ID item"), false);
    bool add1 = service.AddItem(note, itemZero);
    TEST_ASSERT(add1, L"AddItem with ID=0 succeeds");
    TEST_ASSERT(note.items.size() == 1, L"Item was added");
    TEST_ASSERT(note.items[0].uId != 0, L"ID=0 was automatically assigned a non-zero unique timestamp");

    // Test 2: Existing ID update instead of dropping
    uint64_t fixedId = 888888;
    service.AddItem(note, NoteItem(fixedId, _T("Original text"), false));
    service.AddItem(note, NoteItem(fixedId, _T("Updated text"), true));
    TEST_ASSERT(note.items.size() == 2, L"Duplicate ID did not create ghost item");
    TEST_ASSERT(note.items[1].sContent == _T("Updated text"), L"Duplicate ID updated existing item content");
    TEST_ASSERT(note.items[1].bFinished == true, L"Duplicate ID updated finished state");

    // Test 3: Remove item
    bool remOk = service.RemoveItem(note, fixedId);
    TEST_ASSERT(remOk, L"RemoveItem succeeds");
    TEST_ASSERT(note.items.size() == 1, L"Item successfully removed from list and disk");

    // Test 4: Update all items (reordering)
    std::vector<NoteItem> reordered;
    reordered.push_back(NoteItem(9001, _T("First"), false));
    reordered.push_back(NoteItem(9002, _T("Second"), false));
    service.UpdateAllItems(note, reordered);
    TEST_ASSERT(note.items.size() == 2 && note.items[0].uId == 9001, L"UpdateAllItems safely reorders items");

    repo.Delete(noteName);
}

void Test_IPC_MessageDispatcher_Fuzzing()
{
    std::wcout << L"\n[TEST SUITE 5] IPC NoteHostProtocol & MessageDispatcher Fuzzing..." << std::endl;
    NoteHostMessageDispatcher dispatcher;

    bool moveCalled = false;
    struct TestMoveHandler : public INoteHostMessageHandler {
        bool* pCalled;
        explicit TestMoveHandler(bool* c) : pCalled(c) {}
        void Handle(const NoteHostMessage& msg) override {
            if (pCalled) *pCalled = true;
        }
    } moveHandler(&moveCalled);

    dispatcher.Register(NoteHostMessageType::Move, &moveHandler);

    // Mappings
    TEST_ASSERT(NoteHostMessage::TypeFromString(_T("move")) == NoteHostMessageType::Move, L"Host IPC move mapping");
    TEST_ASSERT(NoteHostMessage::TypeFromString(_T("resize")) == NoteHostMessageType::Resize, L"Host IPC resize mapping");
    TEST_ASSERT(NoteHostMessage::TypeFromString(_T("add")) == NoteHostMessageType::Add, L"Host IPC add mapping");
    TEST_ASSERT(NoteHostMessage::TypeFromString(_T("edge_lock")) == NoteHostMessageType::EdgeLock, L"Host IPC edge_lock mapping");

    // Fuzz 1: nullptr - should not crash
    dispatcher.Dispatch(nullptr);
    TEST_ASSERT(!moveCalled, L"Dispatcher safely rejects nullptr string without invoking handler");

    // Fuzz 2: Empty string - should not crash
    dispatcher.Dispatch(_T(""));
    TEST_ASSERT(!moveCalled, L"Dispatcher safely rejects empty string");

    // Fuzz 3: Malformed JSON syntax - should not crash
    dispatcher.Dispatch(_T("{\"event\": \"move\", data: broken..."));
    TEST_ASSERT(!moveCalled, L"Dispatcher safely rejects broken JSON syntax");

    // Fuzz 4: Valid JSON but missing event field - should not crash
    dispatcher.Dispatch(_T("{\"data\": true}"));
    TEST_ASSERT(!moveCalled, L"Dispatcher safely rejects message without event name");

    // Fuzz 5: Unknown event name - should not crash
    dispatcher.Dispatch(_T("{\"event\": \"malicious_unregistered_event\", \"data\": 123}"));
    TEST_ASSERT(!moveCalled, L"Dispatcher safely rejects unknown/unregistered event");

    // Fuzz 6: Valid registered event
    dispatcher.Dispatch(_T("{\"event\": \"move\", \"data\": true}"));
    TEST_ASSERT(moveCalled, L"Dispatcher successfully routes valid registered message to handler");
}

void Test_DataMigrator_Verification()
{
    std::wcout << L"\n[TEST SUITE 6] Legacy INI Data Migration Verification..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage\\"));
    JsonNoteRepository repo(testDir);

    // Create a legacy .ini note
    CString iniPath = Easy::Path::Resolve(testDir, _T("legacy_note.ini"));
    CString iniContent = 
        _T("[Group]\n")
        _T("Count=2\n")
        _T("Name=legacy_note\n")
        _T("Title=Legacy Test Note\n")
        _T("Rect=150,160,600,650\n")
        _T("BgColor=16777215\n")
        _T("Opacity=60\n")
        _T("OpacityAble=1\n")
        _T("TopMost=1\n")
        _T("Visible=1\n")
        _T("\n")
        _T("[Note0]\n")
        _T("Id=1600000000000\n")
        _T("Content=Legacy task 1\n")
        _T("Finished=0\n")
        _T("\n")
        _T("[Note1]\n")
        _T("Id=1600000000001\n")
        _T("Content=Legacy task 2 finished\n")
        _T("Finished=1\n");

    Easy::XFile::WriteFile(iniPath, iniContent);

    DataMigrator::MigrateIfNeeded(repo, true);

    Note migratedNote;
    bool loadOk = repo.Load(_T("legacy_note"), migratedNote);
    TEST_ASSERT(loadOk, L"DataMigrator successfully converted legacy INI to JSON");
    if (loadOk) {
        TEST_ASSERT(migratedNote.title == _T("Legacy Test Note"), L"Migrated title matches");
        TEST_ASSERT(migratedNote.items.size() == 2, L"Migrated all items count");
        TEST_ASSERT(migratedNote.items[0].sContent == _T("Legacy task 1"), L"Migrated item 1 content");
        TEST_ASSERT(migratedNote.items[1].bFinished == true, L"Migrated item 2 finished state");
    }

    // Cleanup
    repo.Delete(_T("legacy_note"));
    DeleteFile(iniPath);
}

void Test_SqliteNoteRepository_CRUD()
{
    std::wcout << L"\n[TEST SUITE 7] SqliteNoteRepository CRUD & 64-bit ID Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_sqlite\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("notes_test.db"));
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));

    SqliteNoteRepository repo(dbPath);

    TEST_ASSERT(repo.ListAll().empty(), L"New SQLite database has empty note list");

    CString noteName = _T("sqlite_note_1");
    Note note(noteName);
    note.title = _T("SQLite Unit Test Note");
    note.rect = CRect(120, 130, 600, 650);
    note.bgColor = RGB(25, 30, 40);
    note.opacity = 80;
    note.opacityEnabled = true;
    note.visible = true;
    note.topMost = true;

    uint64_t id1 = 1724063216000ULL;
    uint64_t id2 = 1724063216999123ULL;
    uint64_t id3 = 0x7FFFFFFFFFFFFFFFULL; // max signed int64
    note.items.push_back(NoteItem(id1, _T("Sqlite Item 1"), false));
    note.items.push_back(NoteItem(id2, _T("Sqlite Item 2"), true));
    note.items.push_back(NoteItem(id3, _T("Sqlite Item 3 (Max Int64)"), false));

    repo.Save(note);

    std::vector<CString> list = repo.ListAll();
    TEST_ASSERT(list.size() == 1 && list[0] == noteName, L"ListAll returns saved note");

    Note loadedNote;
    bool loadOk = repo.Load(noteName, loadedNote);
    TEST_ASSERT(loadOk, L"Load saved note from SQLite");
    if (loadOk) {
        TEST_ASSERT(loadedNote.title == note.title, L"Note title matches");
        TEST_ASSERT(loadedNote.rect == note.rect, L"Note rect matches");
        TEST_ASSERT(loadedNote.bgColor == note.bgColor, L"Note bgColor matches");
        TEST_ASSERT(loadedNote.opacity == 80, L"Note opacity matches");
        TEST_ASSERT(loadedNote.opacityEnabled == true, L"Note opacityEnabled matches");
        TEST_ASSERT(loadedNote.visible == true, L"Note visible matches");
        TEST_ASSERT(loadedNote.topMost == true, L"Note topMost matches");
        TEST_ASSERT(loadedNote.items.size() == 3, L"Note items count matches 3");
        if (loadedNote.items.size() == 3) {
            TEST_ASSERT(loadedNote.items[0].uId == id1, L"Item 1 ID matches");
            TEST_ASSERT(loadedNote.items[1].uId == id2, L"Item 2 ID matches");
            TEST_ASSERT(loadedNote.items[2].uId == id3, L"Item 3 ID matches");
            TEST_ASSERT(loadedNote.items[1].bFinished == true, L"Item 2 finished matches");
        }
    }

    // Rename
    CString newName = _T("sqlite_note_renamed");
    bool renameOk = repo.Rename(noteName, newName);
    TEST_ASSERT(renameOk, L"Rename note in SQLite");
    TEST_ASSERT(!repo.Load(noteName, loadedNote), L"Old name no longer loads");
    TEST_ASSERT(repo.Load(newName, loadedNote) && loadedNote.items.size() == 3, L"New name loads with all items intact");

    // Search
    std::vector<Note> searchResults = repo.SearchByContent(_T("Sqlite Item 2"));
    TEST_ASSERT(searchResults.size() == 1 && searchResults[0].name == newName, L"SearchByContent finds note by item text");

    // Delete
    repo.Delete(newName);
    TEST_ASSERT(repo.ListAll().empty(), L"Delete removes note completely from SQLite");

    repo.Close();
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));
}

void Test_SqliteNoteRepository_UnicodeAndInjection()
{
    std::wcout << L"\n[TEST SUITE 8] SQLite Unicode, Emoji & SQL Injection Safety Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_sqlite\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("notes_security_test.db"));
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));

    SqliteNoteRepository repo(dbPath);

    CString noteName = _T("note_injection_test");
    Note note(noteName);
    note.title = _T("'; DROP TABLE notes; DROP TABLE note_items; --");

    CString content1 = _T("SQL ' OR '1'='1' and \"double quotes\" and \nnewlines \t tabs");
    CString content2 = _T("Unicode: 🌟🔥🎉 中文便签内容 繁體字 日本語 한국어");
    note.items.push_back(NoteItem(5001, content1, false));
    note.items.push_back(NoteItem(5002, content2, true));

    repo.Save(note);

    Note loadedNote;
    bool loadOk = repo.Load(noteName, loadedNote);
    TEST_ASSERT(loadOk, L"Load note with SQL injection attempt payloads");
    if (loadOk) {
        TEST_ASSERT(loadedNote.title == note.title, L"SQL injection payload safely stored as literal title");
        TEST_ASSERT(loadedNote.items.size() == 2, L"Items count preserved");
        TEST_ASSERT(loadedNote.items[0].sContent == content1, L"Quotes and SQL payloads preserved in item content");
        TEST_ASSERT(loadedNote.items[1].sContent == content2, L"Multi-language Unicode and Emojis preserved");
    }

    repo.Close();
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));
}

void Test_SqliteMigrator_Verification()
{
    std::wcout << L"\n[TEST SUITE 9] JSON -> SQLite Migration Verification..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_migration\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("migrated_notes.db"));
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));

    // Create 2 test JSON files in testDir
    CString json1 = Easy::Path::Resolve(testDir, _T("migration_note_1.json"));
    CString json2 = Easy::Path::Resolve(testDir, _T("migration_note_2.json"));

    CString contentJson1 = _T("{\n")
        _T("  \"name\": \"migration_note_1\",\n")
        _T("  \"title\": \"Migrated Note 1\",\n")
        _T("  \"left\": 100, \"top\": 100, \"right\": 500, \"bottom\": 500,\n")
        _T("  \"bgcolor\": \"#112233\",\n")
        _T("  \"opacity\": 75,\n")
        _T("  \"opacityable\": true,\n")
        _T("  \"visible\": true,\n")
        _T("  \"topmost\": true,\n")
        _T("  \"notes\": [\n")
        _T("    { \"id\": 9901, \"content\": \"Migrated task 1\", \"finish\": false },\n")
        _T("    { \"id\": 9902, \"content\": \"Migrated task 2\", \"finish\": true }\n")
        _T("  ]\n")
        _T("}\n");

    CString contentJson2 = _T("{\n")
        _T("  \"name\": \"migration_note_2\",\n")
        _T("  \"title\": \"Migrated Note 2\",\n")
        _T("  \"left\": 200, \"top\": 200, \"right\": 600, \"bottom\": 600,\n")
        _T("  \"bgcolor\": \"#445566\",\n")
        _T("  \"opacity\": 50,\n")
        _T("  \"opacityable\": false,\n")
        _T("  \"visible\": true,\n")
        _T("  \"topmost\": false,\n")
        _T("  \"notes\": []\n")
        _T("}\n");

    Easy::XFile::WriteFile(json1, contentJson1);
    Easy::XFile::WriteFile(json2, contentJson2);

    SqliteNoteRepository repo(dbPath);
    bool migrated = SqliteMigrator::MigrateIfNeeded(repo, testDir, true);
    TEST_ASSERT(migrated, L"SqliteMigrator::MigrateIfNeeded returns true on successful migration");

    std::vector<CString> list = repo.ListAll();
    TEST_ASSERT(list.size() == 2, L"SQLite repo now contains 2 migrated notes");

    Note mNote1;
    if (repo.Load(_T("migration_note_1"), mNote1)) {
        TEST_ASSERT(mNote1.title == _T("Migrated Note 1"), L"Migrated note 1 title matches");
        TEST_ASSERT(mNote1.opacity == 75, L"Migrated note 1 opacity matches");
        TEST_ASSERT(mNote1.items.size() == 2, L"Migrated note 1 items count matches");
        TEST_ASSERT(mNote1.items[1].bFinished == true, L"Migrated note 1 item 2 finish state matches");
    }

    // Verify JSON files backed up
    CString backupDir = Easy::Path::Resolve(testDir, _T("json_backup\\"));
    CString backupJson1 = Easy::Path::Resolve(backupDir, _T("migration_note_1.json"));
    CString backupJson2 = Easy::Path::Resolve(backupDir, _T("migration_note_2.json"));

    TEST_ASSERT(!Easy::Path::Exists(json1), L"Original JSON file 1 removed from note dir");
    TEST_ASSERT(!Easy::Path::Exists(json2), L"Original JSON file 2 removed from note dir");
    TEST_ASSERT(Easy::Path::Exists(backupJson1), L"JSON file 1 moved to json_backup");
    TEST_ASSERT(Easy::Path::Exists(backupJson2), L"JSON file 2 moved to json_backup");

    // Second call should not migrate again since repo is non-empty
    bool secondMigration = SqliteMigrator::MigrateIfNeeded(repo, testDir, true);
    TEST_ASSERT(!secondMigration, L"Second migration call safely skips when DB is already populated");

    repo.Close();
    DeleteFile(backupJson1);
    DeleteFile(backupJson2);
    RemoveDirectory(backupDir);
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));
}

void Test_Manager_Features_And_Search()
{
    std::wcout << L"\n[TEST SUITE 10] Manager IPC & Fulltext Search Capabilities..." << std::endl;

    // 1. Verify all Manager IPC enum mappings
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_list_all")) == NoteManagerMessageType::ListAll, L"IPC mgr_list_all mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_get_note")) == NoteManagerMessageType::GetNote, L"IPC mgr_get_note mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_create_note")) == NoteManagerMessageType::CreateNote, L"IPC mgr_create_note mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_delete_notes")) == NoteManagerMessageType::DeleteNotes, L"IPC mgr_delete_notes mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_toggle_visible")) == NoteManagerMessageType::ToggleVisible, L"IPC mgr_toggle_visible mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_update_note")) == NoteManagerMessageType::UpdateNote, L"IPC mgr_update_note mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_locate_window")) == NoteManagerMessageType::LocateWindow, L"IPC mgr_locate_window mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_export")) == NoteManagerMessageType::Export, L"IPC mgr_export mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_close")) == NoteManagerMessageType::Close, L"IPC mgr_close mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_min")) == NoteManagerMessageType::Min, L"IPC mgr_min mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_max")) == NoteManagerMessageType::Max, L"IPC mgr_max mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_move")) == NoteManagerMessageType::Move, L"IPC mgr_move mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_resize")) == NoteManagerMessageType::Resize, L"IPC mgr_resize mapping");

    // 2. Test Fulltext Search & LoadAllNotes
    CString dbPath = Easy::Path::GetCurDirectory(_T("test_manager.db"));
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));

    SqliteNoteRepository repo(dbPath);

    Note noteA = Note::Create(_T("note_arch"));
    noteA.title = _T("系统架构设计文档");
    noteA.items.push_back(NoteItem(101, _T("编写 SQLite DAL 模块"), true));
    noteA.items.push_back(NoteItem(102, _T("实现 Vue3 管理器工作台"), false));
    repo.Save(noteA);

    Note noteB = Note::Create(_T("note_shopping"));
    noteB.title = _T("周末购物清单");
    noteB.items.push_back(NoteItem(201, _T("购买新鲜蓝莓与牛奶"), true));
    noteB.items.push_back(NoteItem(202, _T("选购办公人体工学椅"), false));
    repo.Save(noteB);

    // Test LoadAllNotes
    std::vector<Note> all = repo.LoadAllNotes();
    TEST_ASSERT(all.size() == 2, L"LoadAllNotes loads all 2 notes with full items");

    // Search by title match
    std::vector<Note> search1 = repo.SearchByContent(_T("架构"));
    TEST_ASSERT(search1.size() == 1 && search1[0].name == _T("note_arch"), L"SearchByContent matches note title keyword");

    // Search by item content match
    std::vector<Note> search2 = repo.SearchByContent(_T("蓝莓"));
    TEST_ASSERT(search2.size() == 1 && search2[0].name == _T("note_shopping"), L"SearchByContent matches item content keyword");

    // Search by non-existent keyword
    std::vector<Note> search3 = repo.SearchByContent(_T("不存在的关键词999"));
    TEST_ASSERT(search3.empty(), L"SearchByContent returns empty for unmatched keyword");

    // Clean up
    repo.Close();
    DeleteFile(dbPath);
    DeleteFile(dbPath + _T("-wal"));
    DeleteFile(dbPath + _T("-shm"));
}

void Test_AppSetting_Serialization_And_Validation()
{
    std::wcout << L"\n[TEST SUITE 11] AppSetting Domain Serialization, Validation & Store Tests..." << std::endl;

    // 1. Default constructor & validation
    AppSetting defaultSetting;
    TEST_ASSERT(defaultSetting.IsValid(), L"Default AppSetting is valid");
    TEST_ASSERT(defaultSetting.bAutoRun == true, L"Default autoRun is true");
    TEST_ASSERT(defaultSetting.sTheme == _T("Default"), L"Default theme is 'Default'");
    TEST_ASSERT(defaultSetting.sDefaultBgColor == _T("#0d1117"), L"Default note background color is '#0d1117'");
    TEST_ASSERT(defaultSetting.sLanguage == _T("zh-CN"), L"Default language is 'zh-CN'");
    TEST_ASSERT(defaultSetting.dwNewHotKey != 0, L"Default new note hotkey is non-zero");
    TEST_ASSERT(defaultSetting.dwEditHotKey != 0, L"Default edit note hotkey is non-zero");

    // 2. ToJson & FromJson Roundtrip
    AppSetting customSetting;
    customSetting.bAutoRun = false;
    customSetting.bCustomWebview2 = true;
    customSetting.sWebview2Path = _T("C:\\runtime\\msedgewebview2.exe");
    customSetting.sNoteDir = _T("D:\\MyNotes\\");
    customSetting.sTheme = _T("Simple");
    customSetting.sDefaultBgColor = _T("#fef3c7");
    customSetting.sLanguage = _T("en-US");
    customSetting.dwNewHotKey = 0x0C4E; // Custom DWORD
    customSetting.dwEditHotKey = 0x0C45;

    CString json = customSetting.ToJson();
    TEST_ASSERT(!json.IsEmpty(), L"AppSetting::ToJson produces non-empty string");

    AppSetting loaded = AppSetting::FromJson(json);
    TEST_ASSERT(loaded.bAutoRun == false, L"FromJson preserves autoRun");
    TEST_ASSERT(loaded.bCustomWebview2 == true, L"FromJson preserves customWebview2");
    TEST_ASSERT(loaded.sWebview2Path == _T("C:\\runtime\\msedgewebview2.exe"), L"FromJson preserves webview2Path");
    TEST_ASSERT(loaded.sNoteDir == _T("D:\\MyNotes\\"), L"FromJson preserves noteDir");
    TEST_ASSERT(loaded.sTheme == _T("Simple"), L"FromJson preserves theme");
    TEST_ASSERT(loaded.sDefaultBgColor == _T("#fef3c7"), L"FromJson preserves defaultBgColor");
    TEST_ASSERT(loaded.sLanguage == _T("en-US"), L"FromJson preserves language 'en-US'");
    TEST_ASSERT(loaded.dwNewHotKey == 0x0C4E, L"FromJson preserves custom newHotKey");
    TEST_ASSERT(loaded.dwEditHotKey == 0x0C45, L"FromJson preserves custom editHotKey");

    // 3. FromJson Resilience on Malformed / Missing Fields
    AppSetting fallbackSetting = AppSetting::FromJson(_T("{\"autorun\": true, \"theme\": \"Dark\", \"invalid_num\": \"not_num\"}"));
    TEST_ASSERT(fallbackSetting.bAutoRun == true, L"FromJson parses lowercase autorun field");
    TEST_ASSERT(fallbackSetting.sTheme == _T("Dark"), L"FromJson parses theme");
    TEST_ASSERT(fallbackSetting.sDefaultBgColor == _T("#0d1117"), L"FromJson falls back to default bgColor when omitted");
    TEST_ASSERT(fallbackSetting.sLanguage == _T("zh-CN"), L"FromJson falls back to default language 'zh-CN' when omitted");

    AppSetting langAliasSetting = AppSetting::FromJson(_T("{\"lang\": \"en-US\"}"));
    TEST_ASSERT(langAliasSetting.sLanguage == _T("en-US"), L"FromJson parses 'lang' alias as language");

    // Nullptr safety
    AppSetting nullSetting = AppSetting::FromJson(nullptr);
    TEST_ASSERT(nullSetting.IsValid(), L"FromJson safely handles nullptr input");
    TEST_ASSERT(nullSetting.sLanguage == _T("zh-CN"), L"FromJson nullptr yields default language 'zh-CN'");

    // 4. AppSettingStore Save and Load INI Persistence
    AppSetting backupSetting = AppSettingStore::Load();
    AppSettingStore::Save(customSetting);
    AppSetting reloadedFromIni = AppSettingStore::Load();
    TEST_ASSERT(reloadedFromIni.sTheme == customSetting.sTheme, L"AppSettingStore preserves theme in INI");
    TEST_ASSERT(reloadedFromIni.bCustomWebview2 == customSetting.bCustomWebview2, L"AppSettingStore preserves customWebview2 in INI");
    TEST_ASSERT(reloadedFromIni.sDefaultBgColor == customSetting.sDefaultBgColor, L"AppSettingStore preserves defaultBgColor in INI");
    TEST_ASSERT(reloadedFromIni.sLanguage == _T("en-US"), L"AppSettingStore preserves language 'en-US' in INI");
    AppSettingStore::Save(backupSetting);
}

void Test_Settings_IPC_MessageRouting()
{
    std::wcout << L"\n[TEST SUITE 12] Settings IPC Event Routing & Fuzzing Tests..." << std::endl;

    // 1. Event Type String Mappings
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_get_app_settings")) == NoteManagerMessageType::GetAppSettings, L"IPC mgr_get_app_settings mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_save_app_settings")) == NoteManagerMessageType::SaveAppSettings, L"IPC mgr_save_app_settings mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_browse_folder")) == NoteManagerMessageType::BrowseFolder, L"IPC mgr_browse_folder mapping");
    TEST_ASSERT(NoteManagerMessage::TypeFromString(_T("mgr_open_folder")) == NoteManagerMessageType::OpenFolder, L"IPC mgr_open_folder mapping");

    // 2. Dispatcher Routing
    NoteManagerMessageDispatcher dispatcher;
    bool saveCalled = false;
    CString receivedJson = _T("");

    struct TestSettingsHandler : public INoteManagerMessageHandler {
        bool* pCalled;
        CString* pJson;
        TestSettingsHandler(bool* c, CString* j) : pCalled(c), pJson(j) {}
        void Handle(const NoteManagerMessage& msg) override {
            if (pCalled) *pCalled = true;
            if (pJson) *pJson = msg.dataJson;
        }
    } saveHandler(&saveCalled, &receivedJson);

    dispatcher.Register(NoteManagerMessageType::SaveAppSettings, &saveHandler);

    // Valid settings save message dispatch
    const wchar_t* validPayload = _T("{\"event\":\"mgr_save_app_settings\",\"data\":{\"autoRun\":true,\"theme\":\"Default\",\"defaultBgColor\":\"#161b22\",\"language\":\"en-US\"}}");
    dispatcher.Dispatch(validPayload);
    TEST_ASSERT(saveCalled, L"Dispatcher routes mgr_save_app_settings message to handler");
    TEST_ASSERT(!receivedJson.IsEmpty(), L"Handler receives settings dataJson payload");

    // Parse received settings data
    AppSetting parsed = AppSetting::FromJson(receivedJson);
    TEST_ASSERT(parsed.bAutoRun == true, L"Handler payload parses valid autoRun");
    TEST_ASSERT(parsed.sDefaultBgColor == _T("#161b22"), L"Handler payload parses valid defaultBgColor");
    TEST_ASSERT(parsed.sLanguage == _T("en-US"), L"Handler payload parses valid language 'en-US'");

    // Fuzzing with malformed payload
    saveCalled = false;
    dispatcher.Dispatch(_T("{\"event\":\"mgr_save_app_settings\",\"data\":{broken json..."));
    TEST_ASSERT(!saveCalled, L"Dispatcher safely rejects broken JSON without invoking settings handler");
}

void Test_NoteEventBus_PubSub()
{
    std::wcout << L"\n[TEST SUITE 13] NoteEventBus Reactive Decoupled Event Bus Tests..." << std::endl;
    NoteEventBus& bus = NoteEventBus::Instance();
    bus.Clear();

    int eventCountA = 0;
    CString lastActionA = _T("");
    CString lastNameA = _T("");

    int eventCountB = 0;
    CString lastActionB = _T("");

    int subA = bus.Subscribe([&](const NoteEvent& ev) {
        eventCountA++;
        lastActionA = ev.TypeToString();
        lastNameA = ev.noteName;
    });

    int subB = bus.Subscribe([&](const NoteEvent& ev) {
        eventCountB++;
        lastActionB = ev.TypeToString();
    });

    // 1. Publish Created Event
    bus.Publish(NoteEvent(NoteEventType::Created, _T("note_test_1")));
    TEST_ASSERT(eventCountA == 1 && eventCountB == 1, L"EventBus delivers Created event to multiple subscribers");
    TEST_ASSERT(lastActionA == _T("create") && lastNameA == _T("note_test_1"), L"EventBus preserves event type and note name");

    // 2. Publish Updated Event
    bus.Publish(NoteEvent(NoteEventType::Updated, _T("note_test_2")));
    TEST_ASSERT(eventCountA == 2 && lastActionA == _T("update") && lastNameA == _T("note_test_2"), L"EventBus delivers Updated event");

    // 3. Unsubscribe subB
    bus.Unsubscribe(subB);
    bus.Publish(NoteEvent(NoteEventType::Deleted, _T("note_test_3")));
    TEST_ASSERT(eventCountA == 3 && eventCountB == 2, L"EventBus correctly handles unsubscription");
    TEST_ASSERT(lastActionA == _T("delete"), L"EventBus delivers Deleted event to remaining subscriber");

    // 4. Clear
    bus.Clear();
    bus.Publish(NoteEvent(NoteEventType::SettingsChanged));
    TEST_ASSERT(eventCountA == 3, L"EventBus Clear prevents further dispatches");
}

void Test_Sqlite_CustomPath_And_SwitchDatabase()
{
    std::wcout << L"\n[TEST SUITE 14] SQLite Custom Path, Checkpoint & SwitchDatabase Tests..." << std::endl;
    CString testDir1 = Easy::Path::GetCurDirectory(_T("test_db_custom_1\\"));
    CString testDir2 = Easy::Path::GetCurDirectory(_T("test_db_custom_2\\"));
    Easy::Path::Create(testDir1);
    Easy::Path::Create(testDir2);

    CString dbPath1 = Easy::Path::Resolve(testDir1, _T("notes.db"));
    CString dbPath2 = Easy::Path::Resolve(testDir2, _T("notes.db"));

    // Cleanup previous artifacts if any
    ::DeleteFile(dbPath1);
    ::DeleteFile(dbPath2);

    // 1. Create repo with custom path 1
    {
        SqliteNoteRepository repo1(dbPath1);
        Note note(_T("path_test_note"));
        note.title = _T("Custom Path Note");
        note.items.push_back(NoteItem(101, _T("Item 1"), false));
        repo1.Save(note);

        TEST_ASSERT(repo1.Checkpoint(), L"Checkpoint WAL data into notes.db");

        // 2. Switch to path 2 (copy target because path 2 doesn't exist)
        bool switchOk = repo1.SwitchDatabase(dbPath2, true);
        TEST_ASSERT(switchOk, L"SwitchDatabase to path 2 succeeds");
        TEST_ASSERT(Easy::Path::Exists(dbPath2), L"Target database file created at path 2");

        // Verify data exists in path 2
        Note loaded;
        bool loadOk = repo1.Load(_T("path_test_note"), loaded);
        TEST_ASSERT(loadOk && loaded.title == _T("Custom Path Note"), L"Data preserved in new database path");
        TEST_ASSERT(loaded.items.size() == 1 && loaded.items[0].sContent == _T("Item 1"), L"Items preserved in new database path");

        // Modify in path 2
        loaded.title = _T("Modified in Path 2");
        repo1.Save(loaded);
    }

    // 3. Verify path 1 remains original and path 2 has the modification
    {
        SqliteNoteRepository repoCheck1(dbPath1);
        Note n1;
        repoCheck1.Load(_T("path_test_note"), n1);
        TEST_ASSERT(n1.title == _T("Custom Path Note"), L"Original database path 1 remains unchanged");

        SqliteNoteRepository repoCheck2(dbPath2);
        Note n2;
        repoCheck2.Load(_T("path_test_note"), n2);
        TEST_ASSERT(n2.title == _T("Modified in Path 2"), L"New database path 2 has updated content");
    }

    // Cleanup
    ::DeleteFile(dbPath1);
    ::DeleteFile(dbPath2);
    ::DeleteFile(Easy::Path::Resolve(testDir1, _T("notes.db-wal")));
    ::DeleteFile(Easy::Path::Resolve(testDir1, _T("notes.db-shm")));
    ::DeleteFile(Easy::Path::Resolve(testDir2, _T("notes.db-wal")));
    ::DeleteFile(Easy::Path::Resolve(testDir2, _T("notes.db-shm")));
}

void Test_Visibility_And_MultiAvenue_Sync()
{
    std::wcout << L"\n[TEST SUITE 15] Visibility & Multi-Avenue Synchronization Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_sync\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("notes.db"));
    ::DeleteFile(dbPath);

    SqliteNoteRepository repo(dbPath);
    NoteService service(repo);

    Note note;
    note.name = _T("SyncTestNote");
    note.title = _T("Sync Note Title");
    note.visible = true;
    note.topMost = false;
    repo.Save(note);

    // 1. Test Hide via NoteService
    service.Hide(note);
    Note loaded;
    TEST_ASSERT(repo.Load(_T("SyncTestNote"), loaded), L"Load note after service.Hide");
    TEST_ASSERT(loaded.visible == false, L"service.Hide sets visible to false in repository");

    // 2. Test SetVisibility(true)
    service.SetVisibility(note, true);
    TEST_ASSERT(repo.Load(_T("SyncTestNote"), loaded), L"Load note after SetVisibility(true)");
    TEST_ASSERT(loaded.visible == true, L"SetVisibility(true) sets visible to true in repository");

    // 3. Test SetVisibility(false)
    service.SetVisibility(note, false);
    TEST_ASSERT(repo.Load(_T("SyncTestNote"), loaded), L"Load note after SetVisibility(false)");
    TEST_ASSERT(loaded.visible == false, L"SetVisibility(false) sets visible to false in repository");

    // Cleanup
    ::DeleteFile(dbPath);
    ::DeleteFile(Easy::Path::Resolve(testDir, _T("notes.db-wal")));
    ::DeleteFile(Easy::Path::Resolve(testDir, _T("notes.db-shm")));
}

void Test_RecycleBin_And_SoftDelete_Lifecycle()
{
    std::wcout << L"\n[TEST SUITE 16] Recycle Bin, Soft-Delete & Schema Migration Tests..." << std::endl;
    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_trash\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("notes_trash_test.db"));
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));

    // 1. Automatic Schema Migration Test (Older table without is_deleted/deleted_at)
    {
        sqlite3* rawDb = nullptr;
        int rc = sqlite3_open16(dbPath.GetString(), &rawDb);
        TEST_ASSERT(rc == SQLITE_OK && rawDb != nullptr, L"Raw SQLite DB created for migration test");

        const char* oldSql =
            "CREATE TABLE notes ("
            "    name        TEXT PRIMARY KEY,"
            "    title       TEXT NOT NULL DEFAULT '',"
            "    win_left    INTEGER NOT NULL DEFAULT 100,"
            "    win_top     INTEGER NOT NULL DEFAULT 100,"
            "    win_right   INTEGER NOT NULL DEFAULT 530,"
            "    win_bottom  INTEGER NOT NULL DEFAULT 530,"
            "    bgcolor     TEXT NOT NULL DEFAULT '#0d1117',"
            "    opacity     INTEGER NOT NULL DEFAULT 50,"
            "    opacity_on  INTEGER NOT NULL DEFAULT 0,"
            "    visible     INTEGER NOT NULL DEFAULT 1,"
            "    topmost     INTEGER NOT NULL DEFAULT 1,"
            "    sort_order  INTEGER NOT NULL DEFAULT 0,"
            "    updated_at  INTEGER NOT NULL DEFAULT 0"
            ");"
            "INSERT INTO notes (name, title, visible) VALUES ('LegacyNote1', 'Legacy Title', 1);";
        sqlite3_exec(rawDb, oldSql, nullptr, nullptr, nullptr);
        sqlite3_close(rawDb);
    }

    // 2. Open with SqliteNoteRepository to trigger EnsureSchema dynamic column migration
    {
        SqliteNoteRepository repo(dbPath);
        std::vector<CString> activeList = repo.ListActive();
        TEST_ASSERT(activeList.size() == 1 && activeList[0] == _T("LegacyNote1"), L"Auto-migrated legacy table maintains active records");

        Note legacyNote;
        TEST_ASSERT(repo.Load(_T("LegacyNote1"), legacyNote), L"Load legacy note successfully");
        TEST_ASSERT(legacyNote.isDeleted == false, L"Legacy note migrated with isDeleted = false");
        TEST_ASSERT(legacyNote.deletedAt == 0, L"Legacy note migrated with deletedAt = 0");

        // 3. Create NoteA, NoteB, NoteC
        Note noteA(_T("NoteA"));
        noteA.title = _T("Alpha Active");
        noteA.items.push_back(NoteItem(101, _T("Item A1"), false));
        repo.Save(noteA);

        Note noteB(_T("NoteB"));
        noteB.title = _T("Beta For Trash");
        noteB.items.push_back(NoteItem(201, _T("Item B1"), false));
        noteB.items.push_back(NoteItem(202, _T("Item B2"), true));
        repo.Save(noteB);

        Note noteC(_T("NoteC"));
        noteC.title = _T("Gamma Active");
        repo.Save(noteC);

        TEST_ASSERT(repo.ListActive().size() == 4, L"4 Active notes before soft delete");
        TEST_ASSERT(repo.ListTrash().empty(), L"Trash is empty initially");

        // 4. Soft Delete NoteB
        NoteService service(repo);
        bool delOk = service.SoftDelete(noteB);
        TEST_ASSERT(delOk, L"service.SoftDelete(noteB) succeeds");
        TEST_ASSERT(noteB.isDeleted == true, L"Domain note.isDeleted updated to true");
        TEST_ASSERT(noteB.visible == false, L"Domain note.visible updated to false on delete");

        // Verify active vs trash lists
        std::vector<CString> activeAfter = repo.ListActive();
        std::vector<CString> trashAfter = repo.ListTrash();
        TEST_ASSERT(activeAfter.size() == 3, L"Active list size reduced by 1");
        TEST_ASSERT(trashAfter.size() == 1 && trashAfter[0] == _T("NoteB"), L"Trash list contains NoteB");

        // Verify Load on deleted note
        Note loadedB;
        TEST_ASSERT(repo.Load(_T("NoteB"), loadedB), L"Load NoteB from SQLite still returns true");
        TEST_ASSERT(loadedB.isDeleted == true, L"Loaded NoteB has isDeleted == true");
        TEST_ASSERT(loadedB.deletedAt > 0, L"Loaded NoteB has valid deletedAt timestamp");
        TEST_ASSERT(loadedB.visible == false, L"Loaded NoteB has visible == false");
        TEST_ASSERT(loadedB.items.size() == 2, L"Loaded NoteB preserves items in trash");

        // Verify SearchByContent excludes deleted notes
        std::vector<Note> searchRes = repo.SearchByContent(_T("Beta"));
        TEST_ASSERT(searchRes.empty(), L"SearchByContent excludes trash notes");

        // 5. Restore NoteB
        bool restOk = service.Restore(noteB);
        TEST_ASSERT(restOk, L"service.Restore(noteB) succeeds");
        TEST_ASSERT(noteB.isDeleted == false, L"Domain note.isDeleted set back to false");
        TEST_ASSERT(noteB.deletedAt == 0, L"Domain note.deletedAt reset to 0");

        TEST_ASSERT(repo.ListActive().size() == 4, L"Active list restored to 4 notes");
        TEST_ASSERT(repo.ListTrash().empty(), L"Trash list is empty after restore");

        // 6. Permanent Delete NoteB
        bool permOk = service.PermanentDelete(_T("NoteB"));
        TEST_ASSERT(permOk, L"service.PermanentDelete(NoteB) succeeds");
        TEST_ASSERT(!repo.Load(_T("NoteB"), loadedB), L"Load NoteB returns false after permanent delete");
        TEST_ASSERT(repo.ListActive().size() == 3, L"Active list has 3 notes");
        TEST_ASSERT(repo.ListTrash().empty(), L"Trash list empty");

        // 7. Clear Trash
        repo.SoftDelete(_T("NoteA"));
        repo.SoftDelete(_T("NoteC"));
        TEST_ASSERT(repo.ListTrash().size() == 2, L"Trash has 2 notes");
        TEST_ASSERT(repo.ListActive().size() == 1, L"Active has 1 note (LegacyNote1)");

        bool clearOk = service.ClearTrash();
        TEST_ASSERT(clearOk, L"service.ClearTrash() succeeds");
        TEST_ASSERT(repo.ListTrash().empty(), L"Trash is empty after ClearTrash");
        TEST_ASSERT(repo.ListActive().size() == 1, L"Active notes untouched by ClearTrash");

        // 8. SQL Injection and Security Checks on Trash API
        TEST_ASSERT(repo.SoftDelete(_T("'; DROP TABLE notes; --")), L"SQL injection payload in SoftDelete handled safely");
        TEST_ASSERT(repo.Restore(_T("' OR '1'='1")), L"SQL injection payload in Restore handled safely");
        TEST_ASSERT(repo.PermanentDelete(_T("non_existent_note")), L"Permanent delete non-existent note handled safely");

        repo.Close();
    }

    // Cleanup
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));
}

void Test_Archive_And_Unarchive_Lifecycle()
{
    std::wcout << L"\n--- Suite 17: Sticky Notes Archive & Unarchive Lifecycle & Smooth Migration ---" << std::endl;

    CString testDir = Easy::Path::GetCurDirectory(_T("test_storage_archive\\"));
    Easy::Path::Create(testDir);
    CString dbPath = Easy::Path::Resolve(testDir, _T("notes_archive_test.db"));
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));

    // 1. Simulate Older Database Schema (without is_archived and archived_at columns)
    {
        sqlite3* rawDb = nullptr;
        int rc = sqlite3_open16(dbPath.GetString(), &rawDb);
        TEST_ASSERT(rc == SQLITE_OK && rawDb != nullptr, L"Raw SQLite DB created for migration test");

        const char* oldSql =
            "CREATE TABLE notes ("
            "    name        TEXT PRIMARY KEY,"
            "    title       TEXT NOT NULL DEFAULT '',"
            "    win_left    INTEGER NOT NULL DEFAULT 100,"
            "    win_top     INTEGER NOT NULL DEFAULT 100,"
            "    win_right   INTEGER NOT NULL DEFAULT 530,"
            "    win_bottom  INTEGER NOT NULL DEFAULT 530,"
            "    bgcolor     TEXT NOT NULL DEFAULT '#0d1117',"
            "    opacity     INTEGER NOT NULL DEFAULT 50,"
            "    opacity_on  INTEGER NOT NULL DEFAULT 0,"
            "    visible     INTEGER NOT NULL DEFAULT 1,"
            "    topmost     INTEGER NOT NULL DEFAULT 1,"
            "    sort_order  INTEGER NOT NULL DEFAULT 0,"
            "    updated_at  INTEGER NOT NULL DEFAULT 0,"
            "    is_deleted  INTEGER NOT NULL DEFAULT 0,"
            "    deleted_at  INTEGER NOT NULL DEFAULT 0"
            ");"
            "INSERT INTO notes (name, title, visible) VALUES ('LegacyNote1', 'Legacy Title', 1);";
        sqlite3_exec(rawDb, oldSql, nullptr, nullptr, nullptr);
        sqlite3_close(rawDb);
    }

    // 2. Open via SqliteNoteRepository to test auto-migration of is_archived & archived_at
    {
        SqliteNoteRepository repo(dbPath);
        NoteService service(repo);

        // Verify that repo initialized and legacy note is active (isArchived defaults to false)
        std::vector<CString> active = repo.ListActive();
        TEST_ASSERT(active.size() == 1 && active[0] == _T("LegacyNote1"), L"Smooth migration preserves active notes");

        std::vector<CString> archived = repo.ListArchived();
        TEST_ASSERT(archived.empty(), L"Archived list is initially empty on migrated database");

        // 3. Create NoteA, NoteB, NoteC
        Note noteA;
        noteA.name = _T("NoteA");
        noteA.title = _T("Active Note Alpha");
        noteA.visible = true;
        noteA.items.push_back({ 1, _T("Task Alpha 1"), false });
        repo.Save(noteA);

        Note noteB;
        noteB.name = _T("NoteB");
        noteB.title = _T("Archivable Note Beta");
        noteB.visible = true;
        noteB.items.push_back({ 2, _T("Task Beta 1"), true });
        noteB.items.push_back({ 3, _T("Task Beta 2"), false });
        repo.Save(noteB);

        Note noteC;
        noteC.name = _T("NoteC");
        noteC.title = _T("Third Note Gamma");
        noteC.visible = true;
        repo.Save(noteC);

        TEST_ASSERT(repo.ListActive().size() == 4, L"4 active notes in repository");
        TEST_ASSERT(repo.ListArchived().empty(), L"0 archived notes");

        // 4. Archive NoteB via NoteService
        bool archOk = service.Archive(noteB);
        TEST_ASSERT(archOk, L"service.Archive(noteB) succeeds");
        TEST_ASSERT(noteB.isArchived == true, L"Domain note.isArchived set to true");
        TEST_ASSERT(noteB.archivedAt > 0, L"Domain note.archivedAt has valid timestamp");
        TEST_ASSERT(noteB.visible == false, L"Domain note.visible automatically set to false on archive");

        // Verify Repository Lists
        std::vector<CString> activeAfter = repo.ListActive();
        std::vector<CString> archivedAfter = repo.ListArchived();
        TEST_ASSERT(activeAfter.size() == 3, L"Active notes count reduced to 3");
        TEST_ASSERT(archivedAfter.size() == 1 && archivedAfter[0] == _T("NoteB"), L"Archived list contains NoteB");

        // Verify Load on archived note
        Note loadedB;
        TEST_ASSERT(repo.Load(_T("NoteB"), loadedB), L"Load NoteB from SQLite returns true");
        TEST_ASSERT(loadedB.isArchived == true, L"Loaded NoteB has isArchived == true");
        TEST_ASSERT(loadedB.archivedAt > 0, L"Loaded NoteB has valid archivedAt timestamp");
        TEST_ASSERT(loadedB.visible == false, L"Loaded NoteB has visible == false");
        TEST_ASSERT(loadedB.items.size() == 2, L"Loaded NoteB preserves items in archive");

        // Verify LoadArchivedNotes()
        std::vector<Note> allArchived = repo.LoadArchivedNotes();
        TEST_ASSERT(allArchived.size() == 1 && allArchived[0].name == _T("NoteB"), L"LoadArchivedNotes returns NoteB");

        // 5. Unarchive NoteB
        bool unarchOk = service.Unarchive(noteB);
        TEST_ASSERT(unarchOk, L"service.Unarchive(noteB) succeeds");
        TEST_ASSERT(noteB.isArchived == false, L"Domain note.isArchived set back to false");
        TEST_ASSERT(noteB.archivedAt == 0, L"Domain note.archivedAt reset to 0");

        TEST_ASSERT(repo.ListActive().size() == 4, L"Active list restored to 4 notes");
        TEST_ASSERT(repo.ListArchived().empty(), L"Archived list is empty after unarchive");

        // 6. Test Archive -> Soft Delete -> Restore transition
        service.Archive(_T("NoteB"));
        TEST_ASSERT(repo.ListArchived().size() == 1, L"NoteB archived again");
        TEST_ASSERT(repo.ListActive().size() == 3, L"Active notes count is 3");

        // Soft delete archived note
        repo.SoftDelete(_T("NoteB"));
        TEST_ASSERT(repo.ListArchived().empty(), L"Soft deleting an archived note removes it from archived list");
        TEST_ASSERT(repo.ListTrash().size() == 1 && repo.ListTrash()[0] == _T("NoteB"), L"Soft deleted archived note appears in trash");

        // Restore from trash
        repo.Restore(_T("NoteB"));
        TEST_ASSERT(repo.ListTrash().empty(), L"Trash list is empty after restore");
        TEST_ASSERT(repo.ListArchived().size() == 1 && repo.ListArchived()[0] == _T("NoteB"), L"Restoring an archived note returns it to the archive");
        TEST_ASSERT(repo.ListActive().size() == 3, L"Active list contains 3 active notes");

        // Unarchive noteB
        service.Unarchive(_T("NoteB"));
        TEST_ASSERT(repo.ListActive().size() == 4, L"Unarchiving restores NoteB to active notes");
        TEST_ASSERT(repo.ListArchived().empty(), L"Archived list is empty after unarchive");

        // 7. Security and Injection Checks
        TEST_ASSERT(repo.Archive(_T("'; DROP TABLE notes; --")), L"SQL injection payload in Archive handled safely");
        TEST_ASSERT(repo.Unarchive(_T("' OR '1'='1")), L"SQL injection payload in Unarchive handled safely");
        TEST_ASSERT(repo.Archive(_T("non_existent_note")), L"Archive non-existent note handled safely");

        repo.Close();
    }

    // Cleanup
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));
}

void Test_PreparedStatementCache_And_NoteDto_DeepRegression()
{
    std::wcout << L"\n--- Suite 18: PreparedStatement Cache, NoteDto Fidelity & DB Compaction ---" << std::endl;
    CString dbPath = Easy::Path::GetCurDirectory(_T("test_suite18.db"));
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));

    {
        SqliteNoteRepository repo(dbPath);

        // 1. High-frequency repetitive writes & reads to stress PreparedStatement Cache
        for (int i = 0; i < 20; ++i)
        {
            CString name;
            name.Format(_T("StressNote_%d"), i);
            Note note(name);
            note.title.Format(_T("Title %d"), i);
            note.items.push_back(NoteItem(1000 + i, _T("Task A"), false));
            note.items.push_back(NoteItem(2000 + i, _T("Task B"), true));
            repo.Save(note);
        }

        std::vector<CString> activeList = repo.ListActive();
        TEST_ASSERT(activeList.size() == 20, L"PreparedStatement cache safely persisted 20 consecutive notes");

        Note checkNote;
        TEST_ASSERT(repo.Load(_T("StressNote_10"), checkNote), L"Cached stmt successfully loaded StressNote_10");
        TEST_ASSERT(checkNote.items.size() == 2, L"StressNote_10 items count intact");
        TEST_ASSERT(checkNote.items[1].bFinished == true, L"StressNote_10 item 2 finished state preserved");

        // 2. NoteDto Serialization & Stringify Fidelity
        RJDoc doc;
        auto& alloc = doc.GetAllocator();
        RJValue vSummary = NoteDto::Summary(checkNote, alloc);
        TEST_ASSERT(vSummary.HasMember(_T("itemCount")) && vSummary[_T("itemCount")].GetInt() == 2, L"NoteDto::Summary computes accurate itemCount");
        TEST_ASSERT(vSummary.HasMember(_T("pendingCount")) && vSummary[_T("pendingCount")].GetInt() == 1, L"NoteDto::Summary computes accurate pendingCount");
        TEST_ASSERT(vSummary.HasMember(_T("completedCount")) && vSummary[_T("completedCount")].GetInt() == 1, L"NoteDto::Summary computes accurate completedCount");

        RJValue vDetail = NoteDto::Detail(checkNote, alloc);
        CString jsonStr = NoteDto::Stringify(vDetail);
        TEST_ASSERT(!jsonStr.IsEmpty(), L"NoteDto::Stringify generates valid non-empty JSON");

        // Roundtrip FromJson
        RJDoc parseDoc;
        parseDoc.Parse(jsonStr.GetString());
        TEST_ASSERT(!parseDoc.HasParseError(), L"NoteDto output parses back without error");
        Note roundtripNote = NoteDto::FromJson(checkNote.name, parseDoc);
        TEST_ASSERT(roundtripNote.title == checkNote.title, L"NoteDto roundtrip preserves note title");
        TEST_ASSERT(roundtripNote.items.size() == checkNote.items.size(), L"NoteDto roundtrip preserves items array");

        // 3. Batch Loading 2-Query Fidelity
        std::vector<Note> allBulkNotes = repo.LoadAllNotes();
        TEST_ASSERT(allBulkNotes.size() == 20, L"LoadAllNotes 2-query batch loaded all 20 notes");

        // 4. DB Compaction & Incremental Vacuum Checkpoint
        TEST_ASSERT(repo.Checkpoint(), L"Checkpoint with incremental vacuum succeeds without error");

        repo.Close();
    }

    // Cleanup
    ::DeleteFile(dbPath);
    ::DeleteFile(dbPath + _T("-wal"));
    ::DeleteFile(dbPath + _T("-shm"));
}

void Test_Database_Export_And_Import_Lifecycle()
{
    std::wcout << L"\n--- Suite 19: Database Online Export & Atomic Overwrite Import Lifecycle ---" << std::endl;

    CString testDir = Easy::Path::GetCurDirectory(_T("test_db_export_import\\"));
    Easy::Path::Create(testDir);
    CString sourceDbPath = testDir + _T("main_notes.db");
    CString backupDbPath = testDir + _T("backup_notes.db");
    CString corruptFilePath = testDir + _T("corrupt_fake.db");

    // 1. Create a database with 2 active notes, 1 archived note, 1 trash note, and items
    {
        SqliteNoteRepository repo(sourceDbPath);

        Note activeNote = Note::Create(_T("ActiveNote_1"));
        activeNote.title = _T("Active Shopping List");
        activeNote.items.emplace_back(1001, _T("Buy milk"), false);
        activeNote.items.emplace_back(1002, _T("Buy eggs"), true);
        repo.Save(activeNote);

        Note activeNote2 = Note::Create(_T("ActiveNote_2"));
        activeNote2.title = _T("Work Sprint");
        activeNote2.items.emplace_back(2001, _T("Deploy v1.0.0"), true);
        repo.Save(activeNote2);

        Note archivedNote = Note::Create(_T("ArchivedNote_1"));
        archivedNote.title = _T("Old 2025 Project");
        archivedNote.isArchived = true;
        archivedNote.archivedAt = 1700000000;
        archivedNote.items.emplace_back(3001, _T("Old task"), true);
        repo.Save(archivedNote);

        Note trashNote = Note::Create(_T("TrashNote_1"));
        trashNote.title = _T("Deleted Draft");
        trashNote.isDeleted = true;
        trashNote.deletedAt = 1710000000;
        repo.Save(trashNote);

        // 2. Perform ExportDatabase
        bool exportRes = repo.ExportDatabase(backupDbPath);
        TEST_ASSERT(exportRes, L"ExportDatabase created valid backup database");
        TEST_ASSERT(Easy::Path::Exists(backupDbPath), L"Backup database file exists on disk");

        repo.Close();
    }

    // 3. Mutate the main database (add new notes, delete old notes, modify items)
    {
        SqliteNoteRepository repo(sourceDbPath);
        Note corruptNote = Note::Create(_T("Unwanted_Note_999"));
        corruptNote.title = _T("I should be wiped out by restore");
        repo.Save(corruptNote);
        repo.PermanentDelete(_T("ActiveNote_1"));

        TEST_ASSERT(repo.ListActive().size() == 2, L"Main database mutated with unwanted records before import");
        repo.Close();
    }

    // 4. Test Corrupt File Import Rejection
    {
        // Write corrupt non-sqlite text into corruptFilePath
        FILE* fp = _wfopen(corruptFilePath.GetString(), L"wb");
        if (fp)
        {
            const char* fakeData = "THIS IS NOT AN SQLITE DATABASE FILE AT ALL";
            fwrite(fakeData, 1, strlen(fakeData), fp);
            fclose(fp);
        }

        SqliteNoteRepository repo(sourceDbPath);
        bool corruptImportRes = repo.ImportDatabase(corruptFilePath);
        TEST_ASSERT(!corruptImportRes, L"ImportDatabase safely rejected corrupt non-SQLite file");

        // Verify main database was untouched
        Note checkUnwanted;
        TEST_ASSERT(repo.Load(_T("Unwanted_Note_999"), checkUnwanted), L"Main database untouched after rejected corrupt import");
        repo.Close();
    }

    // 5. Test Valid Database Import & Overwrite
    {
        SqliteNoteRepository repo(sourceDbPath);
        bool importRes = repo.ImportDatabase(backupDbPath);
        TEST_ASSERT(importRes, L"ImportDatabase successfully imported backup database");

        // Verify that original notes & items are 100% restored
        Note restoredNote1;
        TEST_ASSERT(repo.Load(_T("ActiveNote_1"), restoredNote1), L"Restored ActiveNote_1 from backup");
        TEST_ASSERT(restoredNote1.items.size() == 2, L"Restored ActiveNote_1 items count intact");
        TEST_ASSERT(restoredNote1.items[0].sContent == _T("Buy milk") && !restoredNote1.items[0].bFinished, L"Restored ActiveNote_1 item 0 pending state preserved");
        TEST_ASSERT(restoredNote1.items[1].sContent == _T("Buy eggs") && restoredNote1.items[1].bFinished, L"Restored ActiveNote_1 item 1 finished state preserved");

        // Verify unwanted note is completely wiped out
        Note unwantedCheck;
        TEST_ASSERT(!repo.Load(_T("Unwanted_Note_999"), unwantedCheck), L"Unwanted note completely removed by overwrite import");

        // Verify archived & trash notes restored
        std::vector<CString> archivedList = repo.ListArchived();
        TEST_ASSERT(archivedList.size() == 1 && archivedList[0] == _T("ArchivedNote_1"), L"Archived notes restored intact");

        std::vector<CString> trashList = repo.ListTrash();
        TEST_ASSERT(trashList.size() == 1 && trashList[0] == _T("TrashNote_1"), L"Trash notes restored intact");

        repo.Close();
    }

    // Cleanup test directory
    ::DeleteFile(sourceDbPath);
    ::DeleteFile(sourceDbPath + _T("-wal"));
    ::DeleteFile(sourceDbPath + _T("-shm"));
    ::DeleteFile(backupDbPath);
    ::DeleteFile(corruptFilePath);
    ::RemoveDirectory(testDir);
}

void Test_CLogApp_LogLevel_And_ReleaseFilter()
{
    std::wcout << L"\n--- Suite 20: CLogApp LogLevel Filtering & Release Log Bloat Prevention ---" << std::endl;

    CString testLogDir = Easy::Path::GetCurDirectory(_T("test_log_suite\\"));
    if (!Easy::Path::Exists(testLogDir))
    {
        Easy::Path::Create(testLogDir);
    }
    CString testLogPath = Easy::Path::Resolve(testLogDir, _T("test_error.log"));
    ::DeleteFile(testLogPath);

    // 1. Initialize CLogApp with LOG_FILE and LogLevel::Warn (Release mode behavior)
    CLogApp::Init(LOG_FILE, LogLevel::Warn, testLogPath);
    TEST_ASSERT(CLogApp::GetMinFileLogLevel() == LogLevel::Warn, L"CLogApp initialized with LogLevel::Warn minimum file level");

    // 2. Emit Debug and Info logs - these must NOT be written to the file
    CLogApp::Debug(_T("This is a verbose debug log that should NOT go to file"));
    CLogApp::Info(_T("This is an info log that should NOT go to file"));
    CLogApp::Write(_T("This is a legacy Write log that should NOT go to file"));

    TEST_ASSERT(!Easy::Path::Exists(testLogPath), L"Debug and Info logs did not create or write to log file");

    // 3. Emit Warn, Error, and Fatal logs - these MUST be written to file
    CLogApp::Warn(_T("Warning: Hotkey conflict detected on VK_F1"));
    TEST_ASSERT(Easy::Path::Exists(testLogPath), L"Log file created upon Warning log");

    CString fileContent1;
    Easy::XFile::ReadFile(testLogPath, fileContent1);
    TEST_ASSERT(fileContent1.Find(_T("[WARN]")) != -1 && fileContent1.Find(_T("Hotkey conflict")) != -1,
        L"Log file contains [WARN] tag and warning message");
    TEST_ASSERT(fileContent1.Find(_T("verbose debug log")) == -1,
        L"Log file does NOT contain filtered debug messages");
    TEST_ASSERT(fileContent1.Find(_T("info log that should NOT")) == -1,
        L"Log file does NOT contain filtered info messages");

    CLogApp::Error(_T("Error: Database connection timed out"));
    CLogApp::Fatal(_T("Fatal: Unhandled crash exception 0xC0000005"));

    CString fileContent2;
    Easy::XFile::ReadFile(testLogPath, fileContent2);
    TEST_ASSERT(fileContent2.Find(_T("[ERROR]")) != -1 && fileContent2.Find(_T("Database connection timed out")) != -1,
        L"Log file contains [ERROR] tag and error message");
    TEST_ASSERT(fileContent2.Find(_T("[FATAL]")) != -1 && fileContent2.Find(_T("Unhandled crash exception")) != -1,
        L"Log file contains [FATAL] tag and fatal message");

    // 4. Test dynamic level adjustment (e.g. Debug mode)
    CLogApp::SetMinFileLogLevel(LogLevel::Debug);
    CLogApp::Debug(_T("Debug mode activated: recording verbose traces"));
    CString fileContent3;
    Easy::XFile::ReadFile(testLogPath, fileContent3);
    TEST_ASSERT(fileContent3.Find(_T("[DEBUG]")) != -1 && fileContent3.Find(_T("recording verbose traces")) != -1,
        L"Log file accepts [DEBUG] when min log level is set to Debug");

    // Cleanup
    ::DeleteFile(testLogPath);
    ::RemoveDirectory(testLogDir);
}

int _tmain(int argc, _TCHAR* argv[])
{
    std::wcout << L"================================================================" << std::endl;
    std::wcout << L"   Sticky Notes Full-Lifecycle Reliability & Security Test Suite " << std::endl;
    std::wcout << L"================================================================" << std::endl;

    Test_64BitTimestamp_And_Persistence();
    Test_SpecialCharacters_And_Unicode();
    Test_CorruptedJson_Resilience();
    Test_Service_BusinessRules();
    Test_IPC_MessageDispatcher_Fuzzing();
    Test_DataMigrator_Verification();
    Test_SqliteNoteRepository_CRUD();
    Test_SqliteNoteRepository_UnicodeAndInjection();
    Test_SqliteMigrator_Verification();
    Test_Manager_Features_And_Search();
    Test_AppSetting_Serialization_And_Validation();
    Test_Settings_IPC_MessageRouting();
    Test_NoteEventBus_PubSub();
    Test_Sqlite_CustomPath_And_SwitchDatabase();
    Test_Visibility_And_MultiAvenue_Sync();
    Test_RecycleBin_And_SoftDelete_Lifecycle();
    Test_Archive_And_Unarchive_Lifecycle();
    Test_PreparedStatementCache_And_NoteDto_DeepRegression();
    Test_Database_Export_And_Import_Lifecycle();
    Test_CLogApp_LogLevel_And_ReleaseFilter();

    std::wcout << L"\n================================================================" << std::endl;
    std::wcout << L"   TEST RESULTS: " << g_passCount << L" PASSED, " << g_failCount << L" FAILED" << std::endl;
    std::wcout << L"================================================================" << std::endl;

    return g_failCount == 0 ? 0 : 1;
}
