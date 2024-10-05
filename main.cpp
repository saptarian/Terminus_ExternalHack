#include <wx/textfile.h>
#include <wx/wxprec.h>

#include <chrono>
#include <stdexcept>
#include <utility>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/aboutdlg.h>
#include <wx/notebook.h>
#include <wx/snglinst.h>

#include <memory>

#include "constants.hpp"
#include "custom.hpp"
#include "entry.hpp"
#include "gmanager.hpp"
#include "type.hpp"
#include "util.hpp"

WX_DEFINE_ARRAY_PTR(wxThread*, wxArrayThread);

class MyApp : public wxApp {
 public:
  MyApp()
      : m_gameName("Terminus"),
        m_pyModule("python37.dll"),
        m_pyHookOffset(0xF8700),
        m_pyAssertBytes({0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}) {
    m_shuttingDown = false;
    m_editing = false;
    m_pyRemoteBuffer = nullptr;
    m_pyLongBasePtr = 0;
    m_pyModuleBaseAdr = 0;
    m_character = 0;
    m_gamePid = 0;
  }
  virtual ~MyApp() {}

  virtual bool OnInit() wxOVERRIDE;
  // virtual int OnExit() wxOVERRIDE;

  wxCriticalSection m_critsect;

  std::string m_gameName;
  std::string m_pyModule;
  unsigned m_gamePid;
  uintptr_t m_character;
  uintptr_t m_pyModuleBaseAdr;
  std::unordered_map<std::string, uintptr_t> m_baseCharacters;

  uintptr_t m_pyHookOffset;
  void* m_pyRemoteBuffer;
  ByteArray m_pyAssertBytes;

  uintptr_t m_pyLongBasePtr;
  std::vector<py::PyItem> m_pyItems;
  bool m_editing;

  // all the threads currently alive - as soon as the thread terminates, it's
  // removed from the array
  wxArrayThread m_threads;

  // semaphore used to wait for the threads to exit, see MyFrame::OnQuit()
  wxSemaphore m_semAllDone;

  // indicates that we're shutting down and all threads should exit
  bool m_shuttingDown{false};

 private:
  std::unique_ptr<wxSingleInstanceChecker> m_checker;
};

// worker threads

class MyThread : public wxThread {
 public:
  MyThread(wxFrame* frame) : wxThread(wxTHREAD_DETACHED), m_frame(frame) {}
  virtual ~MyThread();

 protected:
  wxFrame* m_frame;

  virtual wxThread::ExitCode Reject(const wxString& message);
};

class ScanningThread : public MyThread {
 public:
  ScanningThread(wxFrame* frame, bro::Entry* entry, unsigned row = 0)
      : MyThread(frame) {
    m_entry = entry;
    m_row = row;
  }

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;

  bro::Entry* m_entry;
  unsigned m_row;
};

class ScanningAllThread : public MyThread {
 public:
  ScanningAllThread(wxFrame* frame, std::vector<bro::Item>* items)
      : MyThread(frame) {
    m_items = items;
  }

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;

  std::vector<bro::Item>* m_items;
};

class BackgroundScanAll : public MyThread {
 public:
  BackgroundScanAll(wxFrame* frame, std::vector<bro::Item>* items,
                    unsigned nScan = 1)
      : MyThread(frame) {
    m_items = items;
    m_nScan = nScan;
  }

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;

  std::vector<bro::Item>* m_items;
  unsigned m_nScan;
};

class InstallHook : public MyThread {
 public:
  InstallHook(wxFrame* frame) : MyThread(frame) {}

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;
};

class LookupThread : public MyThread {
 public:
  LookupThread(wxFrame* frame) : MyThread(frame) {}

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;
};

class MonitorThread : public MyThread {
 public:
  MonitorThread(wxFrame* frame) : MyThread(frame) {}

 private:
  // thread execution starts here
  virtual void* Entry() wxOVERRIDE;
};

wxDECLARE_EVENT(wxEVT_ABOUT, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_EXIT, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ENABLE_ALL, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DISABLE_ALL, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SAVE_CONFIG, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SAVE_CONFIG_AS, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ADD_ENTRY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_ENSURE_ALL_SCAN, wxCommandEvent);

wxDECLARE_EVENT(wxEVT_LOOKUP_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SCANNING_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SCANNINGALL_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_INSTALL_HOOK_COMPLETED, wxThreadEvent);

wxDECLARE_EVENT(wxEVT_SCANNINGALL_STARTED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_SCANNINGALL_UPDATED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_MONITOR_CHARA_UPDATED, wxThreadEvent);

wxDECLARE_EVENT(wxEVT_THREAD_CANCELED, wxThreadEvent);

class MyFrame : public wxFrame {
 public:
  MyFrame(const wxString& title);
  ~MyFrame();

 private:
  void OnAbout(wxCommandEvent& event);
  void OnExit(wxCommandEvent& event);
  // void OnClose(wxCloseEvent&);

  void OnEnableAll(wxCommandEvent& event);
  void OnDisableAll(wxCommandEvent& event);
  void OnOpenConfig(wxCommandEvent&);
  void OnSaveConfig(wxCommandEvent&);
  void OnSaveConfigAs(wxCommandEvent&);
  void OnAddNewEntry(wxCommandEvent&);
  void OnEnsureScanAll(wxCommandEvent&);

  void OnThreadCancel(wxThreadEvent&);

  void OnLookupCompletion(wxThreadEvent&);
  void OnScanningCompletion(wxThreadEvent&);
  void OnScanningAllCompletion(wxThreadEvent&);
  void OnInstallHookDone(wxThreadEvent&);

  void OnScanningAllStart(wxThreadEvent&);
  void OnScanningAllUpdate(wxThreadEvent&);
  void OnScanCharaUpdate(wxThreadEvent&);

  void DoSaveConfig(const wxString& configPath);
  void DoLoadConfig(const wxString& config);

  void InitEntryCtrl(wxWindow* parent);
  void InitPlayerCtrl(wxWindow* parent);

  void OnItemContextMenu(wxDataViewEvent& event);
  void OnItemValueChanged(wxDataViewEvent& event);
  void OnEditValue(wxDataViewEvent& event);

  void OnStatusBarSize(wxSizeEvent&);

  void DoStartMonitorCharacter();
  void DoStartLookup();
  void DoStartScanAll();
  void DoBackgroundScanning(unsigned nScan);
  void DoInstallHook();
  void DoStartScan(bro::Entry& entry, unsigned row);
  void DoToggle(bro::Item& item, bool enable, unsigned row, unsigned col = 0);

  wxPanel* CreateCharaPage(wxWindow* parent,
                           const std::vector<py::PyItem>& items = {},
                           const std::string& name = "default");

 protected:
  bool m_lookedUp{false};
  std::vector<bro::Item> m_Items;

  friend class ScanningThread;

  bool m_eventFromProgram{false};

 private:
  wxNotebook* m_notebook;
  wxTextCtrl* m_log;
  wxDataViewListCtrl* m_entry_ctrl;

  std::unordered_map<std::string, wxDataViewListCtrl*> m_player_ctrls;

  wxObjectDataPtr<MyListModel> m_model;

  wxGauge* m_progressBar;

  std::unordered_map<std::string, wxControl*> uiComponents;
  bool m_hookInstalled{false};
  bool m_scanAtStage{false};
  const wxString m_title;
};

wxDEFINE_EVENT(wxEVT_ABOUT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_EXIT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ENABLE_ALL, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DISABLE_ALL, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SAVE_CONFIG, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SAVE_CONFIG_AS, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ADD_ENTRY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_ENSURE_ALL_SCAN, wxCommandEvent);

wxDEFINE_EVENT(wxEVT_LOOKUP_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SCANNING_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SCANNINGALL_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_INSTALL_HOOK_COMPLETED, wxThreadEvent);

wxDEFINE_EVENT(wxEVT_SCANNINGALL_STARTED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SCANNINGALL_UPDATED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_MONITOR_CHARA_UPDATED, wxThreadEvent);

wxDEFINE_EVENT(wxEVT_THREAD_CANCELED, wxThreadEvent);

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
  m_checker = std::make_unique<wxSingleInstanceChecker>();

  if (m_checker->IsAnotherRunning()) {
    wxLogError(_("Another program instance is already running, aborting."));
    return false;
  }

  if (!util::EnableDebugPrivilege()) {
    wxLogError(
        _("Failed to enable debug privilege. Try Run as administrator!"));
    return false;
  }

  // if (!util::IsRunningAsAdmin()) {
  //   util::RequestElevation();
  // }

  MyFrame* frame = new MyFrame("Terminus External Hack");
  frame->Show(true);
  return true;
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(400, 500)),
      m_title(title) {
  m_entry_ctrl = NULL;
  m_log = NULL;

  SetIcon(wxICON(sample));

  // Create menus and status bar.
  wxMenu* fileMenu = new wxMenu;
  fileMenu->Append(Id_OpenConfig, "&Load Config...",
                   "Load latest saved on/off state.");
  fileMenu->Append(Id_SaveConfig, "&Save Config", "Save current on/off state.");
  fileMenu->Append(Id_SaveConfigAs, "Save As...", "Save current on/off state.");
  fileMenu->Append(wxID_EXIT, wxEmptyString, "Disable all and quit safely.");

  wxMenu* editMenu = new wxMenu;
  editMenu->Append(Id_EnableAll, "&Enable All");
  editMenu->AppendSeparator();
  // TODO: editMenu->Append(Id_AddNewEntry, "&Add New Entry...");
  editMenu->Append(Id_EnsureScanAll, "Ensure &Scan All",
                   "Ensure every entry working properly.");
  editMenu->AppendSeparator();
  editMenu->Append(Id_DisableAll, "&Disable All");

  wxMenu* helpMenu = new wxMenu;
  helpMenu->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(editMenu, "&Edit");
  menuBar->Append(helpMenu, "&Help");
  this->SetMenuBar(menuBar);

  this->Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
  this->Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
  this->Bind(wxEVT_MENU, &MyFrame::OnOpenConfig, this, Id_OpenConfig);
  this->Bind(wxEVT_MENU, &MyFrame::OnSaveConfig, this, Id_SaveConfig);
  this->Bind(wxEVT_MENU, &MyFrame::OnSaveConfigAs, this, Id_SaveConfigAs);
  this->Bind(wxEVT_MENU, &MyFrame::OnEnableAll, this, Id_EnableAll);
  this->Bind(wxEVT_MENU, &MyFrame::OnDisableAll, this, Id_DisableAll);
  this->Bind(wxEVT_MENU, &MyFrame::OnAddNewEntry, this, Id_AddNewEntry);
  this->Bind(wxEVT_MENU, &MyFrame::OnEnsureScanAll, this, Id_EnsureScanAll);

  CreateStatusBar(2);
  SetStatusText("Loading data", 0);

  // Place the gauge in the second field of the status bar
  wxStatusBar* statusBar = GetStatusBar();
  statusBar->SetMinHeight(20);
  statusBar->SetStatusWidths(2, new int[2]{-1, 100});
  statusBar->SetStatusText(wxEmptyString, 1);
  statusBar->Bind(wxEVT_SIZE, &MyFrame::OnStatusBarSize, this);

  // Create a wxGauge for the progress bar
  m_progressBar = new wxGauge(statusBar, wxID_ANY, 1000, wxPoint(0, 0),
                              wxSize(100, 20), wxGA_HORIZONTAL);
  m_progressBar->Hide();

  wxRect rect;
  statusBar->GetFieldRect(1, rect);

  m_progressBar->SetPosition(rect.GetPosition());
  m_progressBar->SetSize(rect.GetSize());

  // redirect logs from our event handlers to text control
  // m_log = new wxTextCtrl(this, wxID_ANY, wxString(), wxDefaultPosition,
  //                        wxDefaultSize, wxTE_MULTILINE);
  // m_log->SetMinSize(wxSize(-1, 100));
  // wxLog::SetActiveTarget(new wxLogTextCtrl(m_log));

  m_notebook = new wxNotebook(this, wxID_ANY);

  // first page of the notebook
  wxPanel* firstPanel = new wxPanel(m_notebook, wxID_ANY);

  InitEntryCtrl(firstPanel);

  // frame -> mainsizer -> notebook -> panels -> sizer -> [dvctrl]
  wxSizer* firstPanelSz = new wxBoxSizer(wxVERTICAL);

  firstPanelSz->Add(m_entry_ctrl, 1, wxGROW | wxALL, this->FromDIP(5));
  firstPanel->SetSizerAndFit(firstPanelSz);

  // complete GUI
  m_notebook->AddPage(firstPanel, "Entry List");
  m_notebook->AddPage(CreateCharaPage(m_notebook), "Character Editor");

  wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(m_notebook, 1, wxGROW);
  // mainSizer->Add(m_log, 0, wxGROW);

  this->SetMinSize(wxSize(400, 400));
  this->SetSizer(mainSizer);

  this->Bind(wxEVT_THREAD_CANCELED, &MyFrame::OnThreadCancel, this);

  this->Bind(wxEVT_LOOKUP_COMPLETED, &MyFrame::OnLookupCompletion, this);
  this->Bind(wxEVT_SCANNING_COMPLETED, &MyFrame::OnScanningCompletion, this);
  this->Bind(wxEVT_SCANNINGALL_COMPLETED, &MyFrame::OnScanningAllCompletion,
             this);
  this->Bind(wxEVT_INSTALL_HOOK_COMPLETED, &MyFrame::OnInstallHookDone, this);

  this->Bind(wxEVT_SCANNINGALL_STARTED, &MyFrame::OnScanningAllStart, this);
  this->Bind(wxEVT_SCANNINGALL_UPDATED, &MyFrame::OnScanningAllUpdate, this);
  this->Bind(wxEVT_MONITOR_CHARA_UPDATED, &MyFrame::OnScanCharaUpdate, this);

  DoStartLookup();
}

MyFrame::~MyFrame() {
  MyApp& app = wxGetApp();

  if (m_lookedUp) {
    for (auto& item : m_Items) {
      for (auto& rec : item.records) {
        if (rec.Enabled()) rec.Disable(app.m_gamePid);
      }
    }
  }
  {
    wxCriticalSectionLocker locker(app.m_critsect);

    if (auto remoteBuffer = app.m_pyRemoteBuffer) {
      try {
        GameManager game(app.m_gamePid);
        uintptr_t adr = app.m_pyModuleBaseAdr + app.m_pyHookOffset;
        game.WriteProtectedBytes((LPVOID)adr, app.m_pyAssertBytes);
        game.FreeMemory(remoteBuffer);
      } catch (const std::exception& ex) {
        // TODO
      }
    }

    // check if we have any threads running first
    const wxArrayThread& threads = app.m_threads;
    size_t count = threads.GetCount();

    if (!count) return;

    // set the flag indicating that all threads should exit
    app.m_shuttingDown = true;
  }

  // now wait for them to really terminate
  app.m_semAllDone.Wait();
}


namespace err {
const wxString noData = "No data";
const wxString incorrectFormat = "incorrect file format";
}  // namespace err

void MyFrame::InitEntryCtrl(wxWindow* parent) {
  m_entry_ctrl = new MyDataViewListCtrl(parent);

  m_model = new MyListModel();
  m_entry_ctrl->AssociateModel(m_model.get());
  m_entry_ctrl->Disable();

  const auto& items = compiled_json::Entries::get();

  for (const auto& item : items) {
    bro::Item temp{(unsigned)item["id"].get<std::uint64_t>(),
                   std::string(item["name"].get<std::string_view>())};

    for (const auto& rec : item["records"]) {
      temp.records.push_back(
          bro::Entry(std::string(rec["adr_alias"].get<std::string_view>()),
                     std::string(rec["disable"].get<std::string_view>()),
                     std::string(rec["enable"].get<std::string_view>()),
                     std::string(rec["scan"].get<std::string_view>()),
                     rec["offset"].get<int64_t>()));
    }

    m_Items.push_back(temp);
  }

  wxDataViewColumn* const col0 = m_entry_ctrl->AppendToggleColumn("Active");
  col0->SetAlignment(wxALIGN_CENTER);
  col0->SetMinWidth(50);

  wxDataViewColumn* const col1 = m_entry_ctrl->AppendTextColumn("Description");
  col1->SetMinWidth(150);
  col1->SetWidth(200);

  wxDataViewColumn* col2 =
      new wxDataViewColumn("Status", new StatusRenderer(), Col_Status);
  m_entry_ctrl->AppendColumn(col2);
  col2->SetMinWidth(50);

  wxVector<wxVariant> data;

  for (auto& entry : m_Items) {
    data.clear();
    data.push_back(false);
    data.push_back(entry.name);
    data.push_back(wxEmptyString);

    m_entry_ctrl->AppendItem(data);
  }

  m_entry_ctrl->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                     &MyFrame::OnItemContextMenu, this);
  m_entry_ctrl->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                     &MyFrame::OnItemValueChanged, this);

  m_entry_ctrl->Bind(wxEVT_SIZE, [col1, col2, col0, this](wxSizeEvent& e) {
    if (col0->GetWidth() > 100) col0->SetWidth(100);
    if (col2->GetWidth() > 100) col2->SetWidth(100);

    int totalWidth = this->m_entry_ctrl->GetSize().GetWidth();
    col1->SetWidth(totalWidth - col0->GetWidth() - col2->GetWidth() - 30);

    e.Skip();  // Ensure the event is processed further
  });
}

#define max(a, b) (a < b) ? b : a
#define min(a, b) (a > b) ? b : a

wxPanel* MyFrame::CreateCharaPage(wxWindow* parent,
                                  const std::vector<py::PyItem>& items,
                                  const std::string& name) {
  wxPanel* secondPanel = new wxPanel(parent, wxID_ANY);

  wxDataViewListCtrl* ctrl = new wxDataViewListCtrl(secondPanel, wxID_ANY);

  ctrl->AppendTextColumn("Key", wxDATAVIEW_CELL_INERT, 150, wxALIGN_RIGHT)
      ->SetMinWidth(100);
  ctrl->AppendTextColumn("Value", wxDATAVIEW_CELL_INERT, 100, wxALIGN_LEFT)
      ->SetMinWidth(50);

  wxVector<wxVariant> data;

  if (items.empty()) {
    data.clear();
    data.push_back("No data available");
    data.push_back(wxEmptyString);
    ctrl->AppendItem(data);
  } else {
    for (const auto& item : items) {
      wxVariant vvalue;
      if (std::holds_alternative<int>(item.value))
        vvalue = wxVariant(std::get<int>(item.value));
      else if (std::holds_alternative<double>(item.value))
        vvalue = wxVariant(std::get<double>(item.value));
      else
        vvalue = wxVariant(std::get<std::string>(item.value));

      data.clear();
      data.push_back(item.key);
      data.push_back(vvalue.GetString());
      ctrl->AppendItem(data);
    }
  }

  m_player_ctrls[name] = ctrl;

  // Bind double-click event
  m_player_ctrls[name]->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
                             &MyFrame::OnEditValue, this);

  wxSizer* secondPanelSz = new wxBoxSizer(wxVERTICAL);

  secondPanelSz->Add(m_player_ctrls[name], 1, wxGROW | wxALL, this->FromDIP(5));
  secondPanel->SetSizerAndFit(secondPanelSz);

  return secondPanel;
}

void MyFrame::DoStartLookup() {
  if (m_lookedUp) return;

  LookupThread* thread = new LookupThread(this);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thread);
  }

  if (thread->Create() != wxTHREAD_NO_ERROR ||
      thread->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");

  } else {
    this->SetStatusText(wxString::Format("Start %s!", wxGetApp().m_gameName));
  }
}

void MyFrame::DoInstallHook() {
  if (!m_lookedUp || m_hookInstalled) return;

  InstallHook* thr = new InstallHook(this);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thr);
  }

  if (thr->Create() != wxTHREAD_NO_ERROR || thr->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");
  }
}

void MyFrame::DoBackgroundScanning(unsigned nScan) {
  if (!m_lookedUp) return;

  BackgroundScanAll* thr = new BackgroundScanAll(this, &m_Items, nScan);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thr);
  }

  if (thr->Create() != wxTHREAD_NO_ERROR || thr->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");
  }
}

void MyFrame::DoStartScanAll() {
  if (!m_lookedUp) return;

  ScanningAllThread* thr = new ScanningAllThread(this, &m_Items);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thr);
  }

  if (thr->Create() != wxTHREAD_NO_ERROR || thr->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");
  } else {
    m_progressBar->Show();
    m_progressBar->SetValue(0);
  }
}

void MyFrame::DoStartScan(bro::Entry& entry, unsigned row) {
  if (!m_lookedUp) return;

  m_model->SetValue(wxEmptyString, m_entry_ctrl->RowToItem(row), Col_Status);
  m_model->RowValueChanged(row, Col_Status);

  ScanningThread* thr = new ScanningThread(this, &entry, row);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thr);
  }

  if (thr->Create() != wxTHREAD_NO_ERROR || thr->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");
  }
}

void MyFrame::DoToggle(bro::Item& item, bool enable, unsigned row,
                       unsigned col) {
  if (!m_lookedUp) return;

  try {
    if (auto pid = wxGetApp().m_gamePid) {
      unsigned shouldEnabled = 0;

      for (auto& record : item.records) {
        record.Enable(pid, enable);
        if (record.Enabled() == enable) shouldEnabled++;
      }

      if (item.records.size() != shouldEnabled) {
        for (auto& record : item.records) {
          record.Enable(pid, !enable);
        }
        m_eventFromProgram = true;
        m_entry_ctrl->SetToggleValue(!enable, row, col);
      } else {
        m_eventFromProgram = true;
        m_entry_ctrl->SetToggleValue(enable, row, col);
      }
    }
  } catch (const std::exception& ex) {
    this->SetStatusText(wxString::Format("Error: %s.", ex.what()));
  }
}

void MyFrame::DoStartMonitorCharacter() {
  if (!m_lookedUp || !m_hookInstalled) return;

  MonitorThread* thread = new MonitorThread(this);
  {
    wxCriticalSectionLocker enter(wxGetApp().m_critsect);
    wxGetApp().m_threads.Add(thread);
  }

  if (thread->Create() != wxTHREAD_NO_ERROR ||
      thread->Run() != wxTHREAD_NO_ERROR) {
    this->SetStatusText("Could not create thread.");
  }
}

// MyFrame - event handlers

void MyFrame::OnLookupCompletion(wxThreadEvent& WXUNUSED(evt)) {
  this->SetStatusText("Ready");
  this->m_entry_ctrl->Enable();

  m_lookedUp = true;

  // Process id have already found otherwise no way to reach this func
  DoStartScanAll();
}

void MyFrame::OnThreadCancel(wxThreadEvent& evt) {
  this->SetStatusText(evt.GetString());
  // wxLogError(evt.GetString());
  m_progressBar->Hide();
  m_progressBar->SetValue(0);
}

void MyFrame::OnScanningCompletion(wxThreadEvent& evt) {
  ScanPayload payload = evt.GetPayload<ScanPayload>();
  // wxLogMessage("Done scanning %s", payload.name);

  if (payload.isOk) {
    m_model->SetValue("OK", m_entry_ctrl->RowToItem(payload.row), Col_Status);
  } else {
    m_model->SetValue("Fail", m_entry_ctrl->RowToItem(payload.row), Col_Status);
  }
  m_model->RowValueChanged(payload.row, Col_Status);
}

void MyFrame::OnScanningAllCompletion(wxThreadEvent& evt) {
  this->SetStatusText(evt.GetString());
  // wxLogMessage(evt.GetString()); // Ready
  m_progressBar->Hide();
  m_progressBar->SetValue(0);

  DoInstallHook();

  // wxString configPath = wxGetCwd() + "/config.txt";
  // if (!wxFileExists(configPath)) return;

  // wxTextFile configFile(configPath);
  // if (configFile.Open() && !configFile.Eof()) {
  //   DoLoadConfig(configFile.GetFirstLine());
  // }
}

void MyFrame::OnScanningAllStart(wxThreadEvent& WXUNUSED(evt)) {
  for (unsigned row = 0; row < m_Items.size(); row++) {
    m_model->SetValue(wxEmptyString, m_entry_ctrl->RowToItem(row), Col_Status);
    m_model->RowValueChanged(row, Col_Status);
  }
}

void MyFrame::OnScanningAllUpdate(wxThreadEvent& evt) {
  this->m_progressBar->SetValue(evt.GetPayload<ScanPayload>().progress *
                                this->m_progressBar->GetRange());

  return OnScanningCompletion(evt);
}

void MyFrame::OnScanCharaUpdate(wxThreadEvent& evt) {
  MyApp& app = wxGetApp();
  wxCriticalSectionLocker locker(app.m_critsect);

  auto payload =
      evt.GetPayload<std::pair<uintptr_t, std::vector<py::PyItem>>>();
  app.m_pyItems = payload.second;

  if (!m_scanAtStage) {
    m_scanAtStage = true;
    DoBackgroundScanning(2);
  }

  const auto& items = app.m_pyItems;

  // Lookup character name
  std::string characterName, characterOccu, title;
  for (const auto& item : items) {
    if (item.key == "name") {
      characterName = std::get<std::string>(item.value);
    }
    if (item.key == "occu") {
      characterOccu = std::get<std::string>(item.value);
      break;
    }
  }

  if (!characterName.size()) return;

  if (m_player_ctrls.find("default") != m_player_ctrls.end()) {
    unsigned count = m_notebook->GetPageCount();
    m_notebook->GetPage(count - 1)->DestroyChildren();
    m_notebook->DeletePage(count - 1);
    m_player_ctrls.erase("default");
  }

  // TODO : check for character who no longer exists

  // Store character name to address
  title = characterName + "/" + characterOccu;
  app.m_baseCharacters[title] = payload.first;

  if (m_player_ctrls.find(title) != m_player_ctrls.end()) {
    wxDataViewListCtrl* ctrl = m_player_ctrls[title];
    wxVector<wxVariant> data;
    unsigned i = 0;
    for (; i < items.size(); i++) {
      const auto& value = items[i].value;
      wxVariant vvalue;
      if (std::holds_alternative<int>(value))
        vvalue = wxVariant(std::get<int>(value));
      else if (std::holds_alternative<double>(value))
        vvalue = wxVariant(std::get<double>(value));
      else
        vvalue = wxVariant(std::get<std::string>(value));

      // Update existing key-values or append new one
      if (ctrl->RowToItem(i).IsOk()) {
        ctrl->SetTextValue(items[i].key, i, 0);
        ctrl->SetValue(vvalue.GetString(), i, 1);
      } else {
        data.clear();
        data.push_back(items[i].key);
        data.push_back(vvalue.GetString());
        ctrl->AppendItem(data);
      }
    }

    while (ctrl->RowToItem(i).IsOk()) {
      ctrl->DeleteItem(i);
      i++;
    }

    ctrl->Layout();

  } else {
    // Create new panel and append new page
    m_notebook->AddPage(CreateCharaPage(m_notebook, items, title), title);
  }

  // Refresh the layout to reflect changes
  Layout();
}

void MyFrame::OnInstallHookDone(wxThreadEvent& evt) {
  // wxLogMessage("m_character: %08X", evt.GetExtraLong());
  wxGetApp().m_character = (uintptr_t)evt.GetExtraLong();
  m_hookInstalled = true;

  DoStartMonitorCharacter();
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
  wxAboutDialogInfo info;
  info.SetName(m_title);
  info.SetWebSite("https://github.com/saptarian");

  wxAboutBox(info, this);
}

void MyFrame::OnExit(wxCommandEvent& WXUNUSED(event)) { Close(true); }

void MyFrame::OnEnableAll(wxCommandEvent& WXUNUSED(event)) {
  for (size_t row = 0; row < m_Items.size(); row++) {
    DoToggle(m_Items.at(row), true, row);
  }
}

void MyFrame::OnDisableAll(wxCommandEvent& WXUNUSED(event)) {
  for (size_t row = 0; row < m_Items.size(); row++) {
    DoToggle(m_Items.at(row), false, row);
  }
}

void MyFrame::OnOpenConfig(wxCommandEvent& WXUNUSED(event)) {
  wxFileDialog openFileDialog(this, _("Open Config file"), "", "",
                              "Config files (*.txt)|*.txt",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;  // the user changed idea...

  wxString configPath = openFileDialog.GetPath();

  wxTextFile configFile(configPath);
  if (!configFile.Open()) {
    wxMessageBox("Failed to open config file.", "Error", wxICON_ERROR);
    return;
  }

  if (!configFile.Eof()) {
    DoLoadConfig(configFile.GetFirstLine());
  }
}

void MyFrame::DoLoadConfig(const wxString& config) {
  if (!m_lookedUp) return;
  unsigned i = 0;

  for (char c : config) {
    try {
      DoToggle(m_Items.at(i), c == '1', i);
      i++;
    } catch (const std::out_of_range& ex) {
      // TODO
    }
  }
}

void MyFrame::DoSaveConfig(const wxString& configPath) {
  if (!m_lookedUp) return;

  wxTextFile configFile(configPath);

  if (!configFile.Exists()) {
    configFile.Create();
  } else {
    configFile.Open();
    configFile.Clear();
  }

  wxString str('0', m_entry_ctrl->GetItemCount());

  // Iterate through your DataViewListCtrl and save the state
  for (unsigned row = 0; row < str.size(); ++row) {
    // Checkbox active or not
    if (m_entry_ctrl->GetToggleValue(row, 0)) {
      str[row] = '1';
    }
  }

  configFile.AddLine(str);
  configFile.Write();
  configFile.Close();
}

void MyFrame::OnSaveConfig(wxCommandEvent& WXUNUSED(event)) {
  DoSaveConfig(wxGetCwd() + "/config.txt");
}

void MyFrame::OnSaveConfigAs(wxCommandEvent& WXUNUSED(event)) {
  wxFileDialog saveFileDialog(this, _("Save Config file"), "", "",
                              "Config files (*.txt)|*.txt",
                              wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (saveFileDialog.ShowModal() == wxID_CANCEL)
    return;  // the user changed idea...

  DoSaveConfig(saveFileDialog.GetPath());
}

void MyFrame::OnAddNewEntry(wxCommandEvent& WXUNUSED(event)) {
  // TODO
}

void MyFrame::OnEnsureScanAll(wxCommandEvent& WXUNUSED(event)) {
  DoStartScanAll();
}

void MyFrame::OnItemContextMenu(wxDataViewEvent& event) {
  enum { Id_Rescan };
  wxMenu menu;
  menu.Append(Id_Rescan, "&ReScan");

  const wxDataViewItem item = event.GetItem();
  if (!item.IsOk()) return;
  unsigned row = m_model->GetRow(item);

  switch (m_entry_ctrl->GetPopupMenuSelectionFromUser(menu)) {
    case Id_Rescan:
      for (auto& rec : m_Items.at(row).records) {
        DoStartScan(rec, row);
      }
      break;
    case wxID_NONE:
    default:
      return;
  }
}

void MyFrame::OnStatusBarSize(wxSizeEvent& event) {
  wxRect rect;
  if (!GetStatusBar()->GetFieldRect(1, rect)) {
    event.Skip();
    return;
  }

  m_progressBar->SetSize(rect.GetSize());
  m_progressBar->Move(rect.GetPosition());

  event.Skip();
}

void MyFrame::OnEditValue(wxDataViewEvent& event) {
  wxDataViewListCtrl* ctrl =
      dynamic_cast<wxDataViewListCtrl*>(event.GetEventObject());
  int row = ctrl->ItemToRow(event.GetItem());

  MyApp& app = wxGetApp();
  wxCriticalSectionLocker locker(app.m_critsect);
  if (app.m_pyItems.empty()) return;
  auto& item = app.m_pyItems.at(row);
  if (item.key != ctrl->GetTextValue(row, 0)) return;
  if (item.val_type != "float" && item.val_type != "int") return;

  app.m_editing = true;
  wxString newValue = wxGetTextFromUser(
      "Edit Value", "Enter new value:", ctrl->GetTextValue(row, 1), this);

  if (!newValue.IsEmpty() && item.valueAddress && app.m_pyLongBasePtr) {
    if (item.val_type == "float") {
      double dblVal;
      if (newValue.ToDouble(&dblVal) && dblVal >= 0) {
        ctrl->SetValue(wxVariant(dblVal).GetString(), row, 1);

        // Update the memory
        try {
          GameManager(app.m_gamePid)
              .WriteMemory((double*)item.valueAddress, &dblVal);
        } catch (const std::exception& ex) {
          wxMessageBox(ex.what(), "Error (Exception)", wxOK | wxICON_ERROR);
        }

      } else {
        wxMessageBox("Invalid input. Please enter a decimal.", "Error",
                     wxOK | wxICON_ERROR);
      }
    } else {
      int intVal;
      if (newValue.ToInt(&intVal) && intVal >= 0 && intVal <= 255) {
        ctrl->SetValue(wxVariant(intVal).GetString(), row, 1);

        intVal = max(min(intVal, 255), 0);
        py::PyIntObject* pyIntValue =
            (py::PyIntObject*)app.m_pyLongBasePtr + intVal;

        // Update the memory
        try {
          GameManager(app.m_gamePid)
              .WriteMemory((uintptr_t*)item.valueAddress, &pyIntValue);
        } catch (const std::exception& ex) {
          wxMessageBox(ex.what(), "Error (Exception)", wxOK | wxICON_ERROR);
        }
      } else {
        wxMessageBox("Invalid input. Please enter a number 0-255.", "Error",
                     wxOK | wxICON_ERROR);
      }
    }
  }

  app.m_editing = false;
}

void MyFrame::OnItemValueChanged(wxDataViewEvent& event) {
  // Ignore changes coming from our own SetToggleValue() calls below.
  if (m_eventFromProgram) {
    m_eventFromProgram = false;
    return;
  }

  const int rowChanged = m_entry_ctrl->ItemToRow(event.GetItem());

  if (event.GetColumn() != Col_Active) return;

  bool enable = m_entry_ctrl->GetToggleValue(rowChanged, Col_Active);
  DoToggle(m_Items.at(rowChanged), enable, rowChanged, Col_Active);
}

// My Thread

MyThread::~MyThread() {
  MyApp& app = wxGetApp();
  // wxLogMessage("Thread dispatch %u.", this->GetId());

  wxCriticalSectionLocker locker(app.m_critsect);

  wxArrayThread& threads = app.m_threads;
  threads.Remove(this);

  if (threads.IsEmpty()) {
    // signal the main thread that there are no more threads left if it is
    // waiting for us
    if (app.m_shuttingDown) {
      app.m_shuttingDown = false;

      app.m_semAllDone.Post();
    }
  }
}

wxThread::ExitCode MyThread::Reject(const wxString& message) {
  wxThreadEvent* e = new wxThreadEvent(wxEVT_THREAD_CANCELED);
  e->SetString(message);
  wxQueueEvent(m_frame, e->Clone());

  return (wxThread::ExitCode)-1;
}

// ScanningThread

wxThread::ExitCode ScanningThread::Entry() {
  MyApp& app = wxGetApp();
  uintptr_t adr = 0;

  {
    wxCriticalSectionLocker locker(app.m_critsect);

    auto pid = app.m_gamePid;

    if (!pid) return Reject("Could not get process id.");

    if (app.m_shuttingDown) return NULL;

    m_entry->EnsureScan(pid);
    adr = m_entry->Address();
  }

  wxThreadEvent* e = new wxThreadEvent(wxEVT_SCANNING_COMPLETED);
  e->SetPayload<ScanPayload>({adr ? true : false, m_row, m_entry->AdrAlias()});
  wxQueueEvent(m_frame, e->Clone());

  return (wxThread::ExitCode)0;  // success
}

// LookupThread

wxThread::ExitCode LookupThread::Entry() {
  MyApp& app = wxGetApp();

  wxString game_ext(wxString::Format("%s.exe", app.m_gameName));

  while (true) {
    if (this->TestDestroy()) return NULL;

    {
      wxCriticalSectionLocker locker(app.m_critsect);

      if (app.m_shuttingDown) return NULL;
      app.m_gamePid = util::GetProcessID(game_ext.mb_str());
      // Game found, got process Id
      if (app.m_gamePid) break;
    }

    wxThread::Sleep(1000);
  }

  wxThreadEvent* evt = new wxThreadEvent(wxEVT_LOOKUP_COMPLETED);
  wxQueueEvent(m_frame, evt->Clone());

  return (wxThread::ExitCode)0;  // success
}

// BackgroundScanAll

wxThread::ExitCode BackgroundScanAll::Entry() {
  for (unsigned i = 0; i < m_nScan; i++) {
    MyApp& app = wxGetApp();
    if (this->TestDestroy()) return NULL;
    {
      wxCriticalSectionLocker locker(app.m_critsect);
      if (app.m_shuttingDown) return NULL;
    }

    // Do our real loop scanning for every item
    for (auto& item : *m_items) {
      if (this->TestDestroy()) return NULL;
      try {
        wxCriticalSectionLocker locker(app.m_critsect);
        if (app.m_shuttingDown) return NULL;

        // Inner loop for every records in an item
        for (auto& record : item.records) {
          record.EnsureScan(app.m_gamePid);
        }
      } catch (const std::exception& ex) {
        // TODO
      }
      // Delay between entries
      wxThread::Sleep(100);
    }
    // Delay between number of batch
    wxThread::Sleep(1000 * 5);
  }

  return (wxThread::ExitCode)0;  // success
}

// ScanningAllThread

wxThread::ExitCode ScanningAllThread::Entry() {
  const unsigned count = m_items->size();
  if (count < 1) return Reject("There is not item to scan.");

  const unsigned minimumItemOk = (count < 3) ? 1 : 3;
  const auto start = std::chrono::steady_clock::now();

  while (!this->TestDestroy()) {
    wxThreadEvent* e = new wxThreadEvent(wxEVT_SCANNINGALL_STARTED);
    wxQueueEvent(m_frame, e->Clone());

    bool allItemsCompleted = true;
    // Do our real loop scanning for every item
    for (unsigned i = 0; i < count; i++) {
      if (this->TestDestroy()) return NULL;

      unsigned recordCount = m_items->at(i).records.size();
      unsigned adrFound = 0;
      {
        MyApp& app = wxGetApp();

        wxCriticalSectionLocker locker(app.m_critsect);
        if (app.m_shuttingDown) return NULL;

        // Inner loop for every records in an item
        for (auto& record : m_items->at(i).records) {
          try {
            record.EnsureScan(app.m_gamePid);
            // Check scanned records was ok
            if (record.Address()) adrFound++;
          } catch (const std::exception& ex) {
            return Reject(wxString::Format("Error: %s", ex.what()));
          }
        }
      }

      bool itemOK = false;
      // Check every record in an item were ok
      if (adrFound == recordCount) {
        itemOK = true;
      }

      wxThreadEvent* e = new wxThreadEvent(wxEVT_SCANNINGALL_UPDATED);
      e->SetPayload<ScanPayload>(
          {itemOK, i, m_items->at(i).name,
           static_cast<double>(i) / static_cast<double>(count - 2)});
      wxQueueEvent(m_frame, e->Clone());

      // check if first minimum required are OK,
      // otherwise start from beginning
      if (i < minimumItemOk && !itemOK) {
        allItemsCompleted = false;
        break;
      }
    }

    // full scan done isn't it?
    if (allItemsCompleted) break;

    // didn't meet requirement, delay for next try
    wxThread::Sleep(1000);

    // Check for timeout
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >
        60) {  // 60 seconds timeout
      return Reject("Timeout! might the game wasn't fully running");
    }
  }

  wxThreadEvent* e = new wxThreadEvent(wxEVT_SCANNINGALL_COMPLETED);
  e->SetString("Ready");
  wxQueueEvent(m_frame, e->Clone());

  return (wxThread::ExitCode)0;  // success
}

// MonitorThread

// Function to check a specific address in the target process
wxThread::ExitCode MonitorThread::Entry() {
  auto lastUpdate = std::chrono::steady_clock::now();
  const auto updateInterval = std::chrono::seconds(2);

  while (!TestDestroy()) {
    MyApp& app = wxGetApp();
    auto now = std::chrono::steady_clock::now();

    {
      wxCriticalSectionLocker locker(app.m_critsect);
      if (app.m_shuttingDown) return NULL;
    }

    try {
      wxCriticalSectionLocker locker(app.m_critsect);
      if (now - lastUpdate >= updateInterval && !app.m_editing) {
        GameManager game(app.m_gamePid);
        if (uintptr_t charBase =
                game.ReadMemory<uintptr_t>((LPCVOID)app.m_character)) {
          py::CharObj char_Obj =
              game.ReadMemory<py::CharObj>((LPCVOID)charBase);
          PyDictObject char_dictObj =
              game.ReadMemory<PyDictObject>(char_Obj.charVal);

          if (char_dictObj.ma_used > 0) {
            wxThreadEvent* e = new wxThreadEvent(wxEVT_MONITOR_CHARA_UPDATED);
            e->SetPayload(
                std::pair{charBase, py::LookupPyItem(game, char_dictObj)});
            wxQueueEvent(m_frame, e->Clone());
            lastUpdate = now;
          }
        }
      }

    } catch (const std::exception& ex) {
      return Reject(wxString::Format("Error: %s", ex.what()));
    }

    wxThread::Sleep(800);
  }

  return (wxThread::ExitCode)0;  // success
}

// InstallHook

wxThread::ExitCode InstallHook::Entry() {
  uintptr_t ptrCharacter = 0;
  // Assembly code to be injected
  unsigned char asmCode[48] = {
      0x8B, 0x44, 0x24, 0x04,                    // mov eax,[esp+4]
      0x85, 0xC0,                                // test eax,eax
      0x74, 0x28,                                // je short @end
      0x8B, 0x40, 0x04,                          // mov eax,[eax+4]
      0x85, 0xC0,                                // test eax,eax
      0x74, 0x21,                                // je short @end
      0x8B, 0x40, 0x0C,                          // mov eax,[eax+C]
      0x85, 0xC0,                                // test eax,eax
      0x74, 0x1A,                                // je short @end
      0x81, 0x38, 0x43, 0x68, 0x61, 0x72,        // cmp [eax],'Char'
      0x75, 0x12,                                // jne short @end
      0x81, 0x78, 0x04, 0x61, 0x63, 0x74, 0x65,  // cmp [eax+4],'acte'
      0x75, 0x09,                                // jne short @end
      0x8B, 0x44, 0x24, 0x04,                    // mov eax,[esp+4]
      0xA3, 0x56, 0x34, 0x12, 0x00               // mov [0x123456],eax
                                                 // @end:
  };

  MyApp& app = wxGetApp();

  try {
    GameManager game(app.m_gamePid);
    wxCriticalSectionLocker locker(app.m_critsect);

    app.m_pyModuleBaseAdr = game.GetModuleBaseAddress(app.m_pyModule);
    if (!app.m_pyModuleBaseAdr) {
      return Reject("Could not find python module.");
    }

    // Scan for base python type long/int
    std::string pyLongPattern;
    const char* pyLongMask = "?? ?? ?? ?? ?? ?? ?? ?? ";
    pyLongPattern.append(pyLongMask).append("00 00 00 00 00 00 00 00 ");
    pyLongPattern.append(pyLongMask).append("01 00 00 00 01 00 00 00 ");
    pyLongPattern.append(pyLongMask).append("01 00 00 00 02 00 00 00 ");
    pyLongPattern.append(pyLongMask).append("01 00 00 00 03 00 00 00");

    app.m_pyLongBasePtr =
        game.FindPattern(pyLongPattern, app.m_pyModuleBaseAdr);
    if (!app.m_pyLongBasePtr) {
      return Reject("Python type::long pattern was not found.");
    }

    uintptr_t adr = app.m_pyModuleBaseAdr + app.m_pyHookOffset;

    // Install hook
    if (game.ReadBytes((BYTE*)(adr), app.m_pyAssertBytes.size())
            .Compare(app.m_pyAssertBytes)) {
      if (uintptr_t remoteBuffer = (uintptr_t)game.AllocateMemory()) {
        app.m_pyRemoteBuffer = (LPVOID)remoteBuffer;
        // Get the address of the last 4 bytes
        BYTE* lastFourBytes = asmCode + sizeof(asmCode) - sizeof(DWORD);
        // Save an address somewhere in new allocated mem
        ptrCharacter = remoteBuffer + sizeof(asmCode) + 16;
        memcpy(lastFourBytes, &ptrCharacter, sizeof(DWORD));
        unsigned jmpSize = sizeof(DWORD) + sizeof(BYTE);
        game.WriteHook(app.m_pyModuleBaseAdr + app.m_pyHookOffset, remoteBuffer,
                       asmCode, sizeof(asmCode),
                       app.m_pyAssertBytes.size() - jmpSize);
      } else {
        return Reject("Could not allocate memory.");
      }
    } else {
      return Reject("Could not validate module.");
    }
  } catch (const std::exception& ex) {
    return Reject(wxString::Format("Error %s.", ex.what()));
  }

  if (ptrCharacter) {
    wxThreadEvent* e = new wxThreadEvent(wxEVT_INSTALL_HOOK_COMPLETED);
    e->SetExtraLong(ptrCharacter);
    wxQueueEvent(m_frame, e->Clone());
  }

  return (wxThread::ExitCode)0;  // success
}