#ifndef CUSTOM_HPP
#define CUSTOM_HPP

#include <wx/dataview.h>
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

#endif /*CUSTOM_HPP*/