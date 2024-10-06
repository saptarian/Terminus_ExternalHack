#ifndef CUSTOM_HPP
#define CUSTOM_HPP

#include <wx/dataview.h>
#include <wx/dialog.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class MyListModel : public wxDataViewListStore {
 public:
  MyListModel() {}

 private:
};

class MyDataViewListCtrl : public wxDataViewListCtrl {
 public:
  MyDataViewListCtrl(wxWindow *parent);

 private:
};

class StatusRenderer : public wxDataViewCustomRenderer {
 public:
  explicit StatusRenderer();

  virtual bool Render(wxRect rect, wxDC *dc, int state) wxOVERRIDE;

  virtual bool SetValue(const wxVariant &value) wxOVERRIDE;

  virtual wxSize GetSize() const wxOVERRIDE;

  virtual bool GetValue(wxVariant &WXUNUSED(value)) const wxOVERRIDE;

 private:
  wxString m_value;
};

// AboutDialog

class AboutDialog : public wxDialog {
 public:
  AboutDialog(wxWindow *parent, wxWindowID id, const wxString &title);

 private:
  void InitUI();
  const wxString &m_title;
};

#endif /*CUSTOM_HPP*/