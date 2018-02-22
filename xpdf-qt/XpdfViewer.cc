//========================================================================
//
// XpdfViewer.cc
//
// Copyright 2015 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#include <math.h>
#include <QAbstractItemModel>
#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QProgressDialog>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSignalMapper>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include "GString.h"
#include "GList.h"
#include "GlobalParams.h"
#include "QtPDFCore.h"
#include "XpdfApp.h"
#include "XpdfViewer.h"
#include "gmempp.h"

//------------------------------------------------------------------------

static const char *aboutHTML =
  "<h3><img align=\"middle\" src=\"qrc:/xpdf-icon\"> "
  "Xpdf"
  "</h3>"
  "Version " xpdfVersion "<br>"
  "<br>"
  "<a href=\"http://www.xpdfreader.com/\">www.xpdfreader.com</a><br>"
  "<br>"
  "Based on the <b>XpdfWidget/Qt</b> toolkit from Glyph & Cog.<br>"
  "For information on commercial licensing:<br>"
  "<a href=\"http://www.glyphandcog.com/XpdfWidgetQt.html\">www.glyphandcog.com/XpdfWidgetQt.html</a><br>"
  "<br>"
  xpdfCopyright ".<br>"
  "Xpdf is licensed under the GNU General Public License (GPL), version 2 or 3.<br>"
  "<hr><br>"
  "The PDF data structures, operators, and specification are documented in ISO 32000-1:2008.<br>"
  "<br>"
  "XpdfReader uses the following open source libraries:"
  "<ul>"
  "<li>Portions of this software are Copyright 2006-2017 The FreeType Project.  All rights reserved.  FreeType is used here under the terms of the FreeType Project License."
  "<li>The Qt Toolkit is Copyright 2015 The Qt Company Ltd.  Qt is used here under the terms of the LGPL v2.1."
  "</ul>";

const char *helpURL = "http://www.xpdfreader.com/help";

//------------------------------------------------------------------------

#define nZoomComboBoxVals 13
static int zoomComboBoxVals[nZoomComboBoxVals] = {
  25, 50, 75, 100, 110, 125, 150, 175, 200, 300, 400, 600, 800
};

#define maxZoom 2000

// Maximum number of errors to show in the error window.  Beyond this
// limit, old errors are removed from the window.
#define errorWindowMaxErrors 100

//------------------------------------------------------------------------
// command table
//------------------------------------------------------------------------

#define cmdMaxArgs 8

//                                |- requires -|
//  command                nArgs  doc     event   function
XpdfViewerCmd XpdfViewer::cmdTab[] = {
  { "about",                   0, gFalse, gFalse, &XpdfViewer::cmdAbout },
  { "blockSelectMode",         0, gFalse, gFalse, &XpdfViewer::cmdBlockSelectMode },
  { "closeSidebar",            0, gFalse, gFalse, &XpdfViewer::cmdCloseSidebar },
  { "closeSidebarMoveResizeWin",   0, gFalse, gFalse, &XpdfViewer::cmdCloseSidebarMoveResizeWin },
  { "closeSidebarResizeWin",   0, gFalse, gFalse, &XpdfViewer::cmdCloseSidebarResizeWin },
  { "closeTabOrQuit",          0, gFalse, gFalse, &XpdfViewer::cmdCloseTabOrQuit },
  { "closeWindowOrQuit",       0, gFalse, gFalse, &XpdfViewer::cmdCloseWindowOrQuit },
  { "continuousMode",          0, gFalse, gFalse, &XpdfViewer::cmdContinuousMode },
  { "copy",                    0, gFalse, gFalse, &XpdfViewer::cmdCopy },
#if 0 // for debugging
  { "debug1",                  0, gTrue,  gTrue,  &XpdfViewer::cmdDebug1 },
#endif
  { "endPan",                  0, gTrue,  gTrue,  &XpdfViewer::cmdEndPan },
  { "endSelection",            0, gTrue,  gTrue,  &XpdfViewer::cmdEndSelection },
  { "find",                    0, gTrue,  gFalse, &XpdfViewer::cmdFind },
  { "findFirst",               0, gTrue,  gFalse, &XpdfViewer::cmdFindFirst },
  { "findNext",                0, gTrue,  gFalse, &XpdfViewer::cmdFindNext },
  { "findPrevious",            0, gTrue,  gFalse, &XpdfViewer::cmdFindPrevious },
  { "focusToDocWin",           0, gFalse, gFalse, &XpdfViewer::cmdFocusToDocWin },
  { "focusToPageNum",          0, gFalse, gFalse, &XpdfViewer::cmdFocusToPageNum },
  { "followLink",              0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLink },
  { "followLinkInNewTab",      0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLinkInNewTab },
  { "followLinkInNewTabNoSel", 0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLinkInNewTabNoSel },
  { "followLinkInNewWin",      0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLinkInNewWin },
  { "followLinkInNewWinNoSel", 0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLinkInNewWinNoSel },
  { "followLinkNoSel",         0, gTrue,  gTrue,  &XpdfViewer::cmdFollowLinkNoSel },
  { "fullScreenMode",          0, gFalse, gFalse, &XpdfViewer::cmdFullScreenMode },
  { "goBackward",              0, gFalse, gFalse, &XpdfViewer::cmdGoBackward },
  { "goForward",               0, gFalse, gFalse, &XpdfViewer::cmdGoForward },
  { "gotoDest",                1, gTrue,  gFalse, &XpdfViewer::cmdGotoDest },
  { "gotoLastPage",            0, gTrue,  gFalse, &XpdfViewer::cmdGotoLastPage },
//~   { "gotoLastPageNoScroll",    0, gTrue,  gFalse, &XpdfViewer::cmdGotoLastPageNoScroll },
  { "gotoPage",                1, gTrue,  gFalse, &XpdfViewer::cmdGotoPage },
//~   { "gotoPageNoScroll",        1, gTrue,  gFalse, &XpdfViewer::cmdGotoPageNoScroll },
  { "help",                    0, gFalse, gFalse, &XpdfViewer::cmdHelp },
  { "horizontalContinuousMode",0, gFalse, gFalse, &XpdfViewer::cmdHorizontalContinuousMode },
  { "linearSelectMode",        0, gFalse, gFalse, &XpdfViewer::cmdLinearSelectMode },
  { "newTab",                  0, gFalse, gFalse, &XpdfViewer::cmdNewTab },
  { "newWindow",               0, gFalse, gFalse, &XpdfViewer::cmdNewWindow },
  { "nextPage",                0, gTrue,  gFalse, &XpdfViewer::cmdNextPage },
  { "nextPageNoScroll",        0, gTrue,  gFalse, &XpdfViewer::cmdNextPageNoScroll },
  { "nextTab",                 0, gTrue,  gFalse, &XpdfViewer::cmdNextTab },
  { "open",                    0, gFalse, gFalse, &XpdfViewer::cmdOpen },
  { "openErrorWindow",         0, gFalse, gFalse, &XpdfViewer::cmdOpenErrorWindow },
//~   { "openFile",                1, gFalse, gFalse, &XpdfViewer::cmdOpenFile },
//~   { "openFileAtDest",          2, gFalse, gFalse, &XpdfViewer::cmdOpenFileAtDest },
//~   { "openFileAtDestInNewWin",  2, gFalse, gFalse, &XpdfViewer::cmdOpenFileAtDestInNewWin },
//~   { "openFileAtPage",          2, gFalse, gFalse, &XpdfViewer::cmdOpenFileAtPage },
//~   { "openFileAtPageInNewWin",  2, gFalse, gFalse, &XpdfViewer::cmdOpenFileAtPageInNewWin },
//~   { "openFileInNewWin",        1, gFalse, gFalse, &XpdfViewer::cmdOpenFileInNewWin },
//~   { "openInNewWin",            0, gFalse, gFalse, &XpdfViewer::cmdOpenInNewWin },
  { "openSidebar",             0, gFalse, gFalse, &XpdfViewer::cmdOpenSidebar },
  { "openSidebarMoveResizeWin",    0, gFalse, gFalse, &XpdfViewer::cmdOpenSidebarMoveResizeWin },
  { "openSidebarResizeWin",    0, gFalse, gFalse, &XpdfViewer::cmdOpenSidebarResizeWin },
  { "pageDown",                0, gTrue,  gFalse, &XpdfViewer::cmdPageDown },
  { "pageUp",                  0, gTrue,  gFalse, &XpdfViewer::cmdPageUp },
  { "postPopupMenu",           0, gFalse, gTrue,  &XpdfViewer::cmdPostPopupMenu },
  { "prevPage",                0, gTrue,  gFalse, &XpdfViewer::cmdPrevPage },
  { "prevPageNoScroll",        0, gTrue,  gFalse, &XpdfViewer::cmdPrevPageNoScroll },
  { "prevTab",                 0, gTrue,  gFalse, &XpdfViewer::cmdPrevTab },
#if XPDFWIDGET_PRINTING
  { "print",                   0, gTrue,  gFalse, &XpdfViewer::cmdPrint },
#endif
  { "quit",                    0, gFalse, gFalse, &XpdfViewer::cmdQuit },
//~   { "raise",                   0, gFalse, gFalse, &XpdfViewer::cmdRaise },
//~   { "redraw",                  0, gTrue,  gFalse, &XpdfViewer::cmdRedraw },
  { "reload",                  0, gTrue,  gFalse, &XpdfViewer::cmdReload },
  { "rotateCCW",               0, gTrue,  gFalse, &XpdfViewer::cmdRotateCCW },
  { "rotateCW",                0, gTrue,  gFalse, &XpdfViewer::cmdRotateCW },
  { "run",                     1, gFalse, gFalse, &XpdfViewer::cmdRun },
  { "saveAs",                  0, gTrue,  gFalse, &XpdfViewer::cmdSaveAs },
  { "saveImage",               0, gTrue,  gFalse, &XpdfViewer::cmdSaveImage },
  { "scrollDown",              1, gTrue,  gFalse, &XpdfViewer::cmdScrollDown },
  { "scrollDownNextPage",      1, gTrue,  gFalse, &XpdfViewer::cmdScrollDownNextPage },
  { "scrollLeft",              1, gTrue,  gFalse, &XpdfViewer::cmdScrollLeft },
  { "scrollOutlineDown",       1, gTrue,  gFalse, &XpdfViewer::cmdScrollOutlineDown },
  { "scrollOutlineUp",         1, gTrue,  gFalse, &XpdfViewer::cmdScrollOutlineUp },
  { "scrollRight",             1, gTrue,  gFalse, &XpdfViewer::cmdScrollRight },
  { "scrollToBottomEdge",      0, gTrue,  gFalse, &XpdfViewer::cmdScrollToBottomEdge },
  { "scrollToBottomRight",     0, gTrue,  gFalse, &XpdfViewer::cmdScrollToBottomRight },
  { "scrollToLeftEdge",        0, gTrue,  gFalse, &XpdfViewer::cmdScrollToLeftEdge },
  { "scrollToRightEdge",       0, gTrue,  gFalse, &XpdfViewer::cmdScrollToRightEdge },
  { "scrollToTopEdge",         0, gTrue,  gFalse, &XpdfViewer::cmdScrollToTopEdge },
  { "scrollToTopLeft",         0, gTrue,  gFalse, &XpdfViewer::cmdScrollToTopLeft },
  { "scrollUp",                1, gTrue,  gFalse, &XpdfViewer::cmdScrollUp },
  { "scrollUpPrevPage",        1, gTrue,  gFalse, &XpdfViewer::cmdScrollUpPrevPage },
  { "setSelection",            5, gTrue,  gFalse, &XpdfViewer::cmdSetSelection },
  { "sideBySideContinuousMode",0, gFalse, gFalse, &XpdfViewer::cmdSideBySideContinuousMode },
  { "sideBySideSingleMode",    0, gFalse, gFalse, &XpdfViewer::cmdSideBySideSingleMode },
  { "singlePageMode",          0, gFalse, gFalse, &XpdfViewer::cmdSinglePageMode },
  { "startPan",                0, gTrue,  gTrue,  &XpdfViewer::cmdStartPan },
  { "startSelection",          0, gTrue,  gTrue,  &XpdfViewer::cmdStartSelection },
  { "toggleContinuousMode",    0, gFalse, gFalse, &XpdfViewer::cmdToggleContinuousMode },
  { "toggleFullScreenMode",    0, gFalse, gFalse, &XpdfViewer::cmdToggleFullScreenMode },
  { "toggleSelectMode",        0, gFalse, gFalse, &XpdfViewer::cmdToggleSelectMode },
  { "toggleSidebar",           0, gFalse, gFalse, &XpdfViewer::cmdToggleSidebar },
  { "toggleSidebarMoveResizeWin",  0, gFalse, gFalse, &XpdfViewer::cmdToggleSidebarMoveResizeWin },
  { "toggleSidebarResizeWin",  0, gFalse, gFalse, &XpdfViewer::cmdToggleSidebarResizeWin },
  { "windowMode",              0, gFalse, gFalse, &XpdfViewer::cmdWindowMode },
  { "zoomFitPage",             0, gFalse, gFalse, &XpdfViewer::cmdZoomFitPage },
  { "zoomFitWidth",            0, gFalse, gFalse, &XpdfViewer::cmdZoomFitWidth },
  { "zoomIn",                  0, gFalse, gFalse, &XpdfViewer::cmdZoomIn },
  { "zoomOut",                 0, gFalse, gFalse, &XpdfViewer::cmdZoomOut },
  { "zoomPercent",             1, gFalse, gFalse, &XpdfViewer::cmdZoomPercent },
  { "zoomToSelection",         0, gTrue,  gFalse, &XpdfViewer::cmdZoomToSelection }
};

#define nCmds (sizeof(cmdTab) / sizeof(XpdfViewerCmd))

//------------------------------------------------------------------------
// XpdfMenuButton
//------------------------------------------------------------------------

XpdfMenuButton::XpdfMenuButton(QMenu *menuA) {
  menu = menuA;
  connect(this, SIGNAL(pressed()), this, SLOT(btnPressed()));
}

void XpdfMenuButton::btnPressed() {
  QSize menuSize = menu->sizeHint();
  QPoint pos = mapToGlobal(QPoint(width(), height()));
  pos -= QPoint(menuSize.width(), 0);
  menu->exec(pos);
  setDown(false);
  setAttribute(Qt::WA_UnderMouse, false);
}

//------------------------------------------------------------------------
// XpdfErrorWindow
//------------------------------------------------------------------------

class XpdfErrorEvent: public QEvent {
public:

  XpdfErrorEvent(int eventType, QString msgA):
    QEvent((Type)eventType), msg(msgA) {}
  QString getMessage() { return msg; }

private:

  QString msg;
};

XpdfErrorWindow::XpdfErrorWindow(XpdfViewer *viewerA, int errorEventTypeA) {
  viewer = viewerA;
  errorEventType = errorEventTypeA;

  QVBoxLayout *topLayout = new QVBoxLayout();

  QHBoxLayout *btnLayout = new QHBoxLayout();
  topLayout->addLayout(btnLayout);

  QPushButton *clearBtn = new QPushButton("Clear");
  connect(clearBtn, SIGNAL(clicked()), this, SLOT(clearBtnPressed()));
  btnLayout->addWidget(clearBtn);

  btnLayout->addStretch(1);

  list = new QListWidget();
  topLayout->addWidget(list);

  setLayout(topLayout);

  lastSize = QSize(list->fontMetrics().width("m") * 50,
		   list->fontMetrics().lineSpacing() * 16);

  setErrorCallback(&errorCbk, this);
}

XpdfErrorWindow::~XpdfErrorWindow() {
  // If the user quits while a page is rendering, we want any
  // subsequent error messages to vanish -- they shouldn't try to
  // update the error window (which has been deleted), and they
  // shouldn't get dumped to stderr.
  setErrorCallback(dummyErrorCbk, NULL);
}

QSize XpdfErrorWindow::sizeHint() const {
  return lastSize;
}

void XpdfErrorWindow::clearBtnPressed() {
  list->clear();
  viewer->statusIndicatorOk();
}

void XpdfErrorWindow::closeEvent(QCloseEvent *event) {
  lastSize = size();
}

void XpdfErrorWindow::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    close();
  }
}

void XpdfErrorWindow::errorCbk(void *data, ErrorCategory category,
			       int pos, char *msg) {
  XpdfErrorWindow *errWin = (XpdfErrorWindow *)data;
  GString *s;

  if (pos >= 0) {
    s = GString::format("{0:s} ({1:d}): {2:s}",
			errorCategoryNames[category], pos, msg);
  } else {
    s = GString::format("{0:s}: {1:s}",
			errorCategoryNames[category], msg);
  }
  XpdfApp::postEvent(errWin, new XpdfErrorEvent(errWin->errorEventType,
						s->getCString()));
  delete s;
}

void XpdfErrorWindow::dummyErrorCbk(void *data, ErrorCategory category,
				    int pos, char *msg) {
}

void XpdfErrorWindow::customEvent(QEvent *event) {
  XpdfErrorEvent *errEvent;

  if (event->type() == errorEventType) {
    errEvent = (XpdfErrorEvent *)event;
    if (list->count() < errorWindowMaxErrors) {
      list->addItem(errEvent->getMessage());
      list->scrollToBottom();
      viewer->statusIndicatorError();
    } else if (list->count() == errorWindowMaxErrors) {
      list->addItem("... additional errors not logged ...");
      list->scrollToBottom();
      viewer->statusIndicatorError();
    }
  }
}

//------------------------------------------------------------------------
// ZoomValidator
//------------------------------------------------------------------------

class ZoomValidator: public QValidator {
public:

  ZoomValidator(QObject *parent = NULL): QValidator(parent) {}
  virtual State validate(QString &input, int &pos) const;
  virtual void fixup(QString &input) const;
};

QValidator::State ZoomValidator::validate(QString &input, int &pos) const {
  QChar c;
  int n, i;

  n = input.length();
  if (n == 0) {
    return QValidator::Intermediate;
  }
  for (i = 0; i < n - 1; ++i) {
    c = input[i];
    if (c < '0' || c > '9') {
      return QValidator::Invalid;
    }
  }
  c = input[n - 1];
  if (c == '%') {
    if (n > 1) {
      return QValidator::Acceptable;
    }
    return QValidator::Intermediate;
  }
  if (c < '0' || c > '9') {
    return QValidator::Invalid;
  }
  return QValidator::Intermediate;
}

void ZoomValidator::fixup(QString &input) const {
  if (!input.endsWith("%")) {
    input.append('%');
  }
}

//------------------------------------------------------------------------
// PropertyListAnimation
//------------------------------------------------------------------------

class PropertyListAnimation: public QPropertyAnimation {
public:

  PropertyListAnimation(QObject *target, const QByteArray &propertyName,
			QList<QVariant> valueListA, QObject *parent = 0):
    QPropertyAnimation(target, propertyName, parent), valueList(valueListA) {}

  virtual QVariant interpolated(const QVariant &from, const QVariant &to,
				qreal progress) const;

  QList<QVariant> values() { return valueList; }
  void setValues(QList<QVariant> valueListA);

private:

  QList<QVariant> valueList;
};

QVariant PropertyListAnimation::interpolated(const QVariant &from,
					     const QVariant &to,
					     qreal progress) const {
  int i;

  i = (int)(progress * valueList.size());
  if (i < 0) {
    i = 0;
  } else if (i >= valueList.size()) {
    i = valueList.size() - 1;
  }
  return valueList[i];
}

void PropertyListAnimation::setValues(QList<QVariant> valueListA) {
  qreal progress;

  valueList = valueListA;
  progress = easingCurve().valueForProgress(qreal(currentTime())
					    / qreal(totalDuration()));
  updateCurrentValue(interpolated(0, 0, progress));
}

//------------------------------------------------------------------------
// OutlineModel
//------------------------------------------------------------------------

class OutlineModel: public QAbstractItemModel {
public:

  OutlineModel(XpdfWidget *pdfA);
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &par = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &idx) const;
  virtual int rowCount(const QModelIndex &par = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &par = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &idx, int role) const;

  QModelIndex findPageIndex(int pg, QTreeView *tree,
			    const QModelIndex &idx = QModelIndex());

  void beginOpenNewDoc();
  void endOpenNewDoc();

private:

  int getItemRow(XpdfOutlineHandle item) const;
  XpdfWidget *pdf;
};

OutlineModel::OutlineModel(XpdfWidget *pdfA) {
  pdf = pdfA;
}

QModelIndex OutlineModel::index(int row, int column,
				const QModelIndex &par) const {
  XpdfOutlineHandle item;
  int nChildren;

  if (par.isValid()) {
    // children of an outline item
    item = (XpdfOutlineHandle)par.internalPointer();
  } else {
    // root outline items
    item = NULL;
  }
  nChildren = pdf->getOutlineNumChildren(item);
  if (row < 0 || row >= nChildren || column != 0) {
    return QModelIndex();
  }
  return createIndex(row, 0, pdf->getOutlineChild(item, row));
}

QModelIndex OutlineModel::parent(const QModelIndex &idx) const {
  XpdfOutlineHandle item, par;
  int row;

  if (!idx.isValid()) {
    return QModelIndex();
  }
  item = (XpdfOutlineHandle)idx.internalPointer();
  if (!(par = pdf->getOutlineParent(item))) {
    return QModelIndex();
  }
  if ((row = getItemRow(par)) < 0) {
    return QModelIndex();
  }
  return createIndex(row, 0, par);
}

int OutlineModel::rowCount(const QModelIndex &par) const {
  XpdfOutlineHandle item;

  if (par.isValid()) {
    // children of an outline item
    item = (XpdfOutlineHandle)par.internalPointer();
  } else {
    // root outline items
    item = NULL;
  }
  return pdf->getOutlineNumChildren(item);
}

int OutlineModel::columnCount(const QModelIndex &par) const {
  return 1;
}

QVariant OutlineModel::data(const QModelIndex &idx, int role) const {
  XpdfOutlineHandle item;

  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  item = (XpdfOutlineHandle)idx.internalPointer();
  return QVariant(pdf->getOutlineTitle(item));
}

int OutlineModel::getItemRow(XpdfOutlineHandle item) const {
  XpdfOutlineHandle par;
  int nChildren, i;

  par = pdf->getOutlineParent(item);
  nChildren = pdf->getOutlineNumChildren(par);
  for (i = 0; i < nChildren; ++i) {
    if (item == pdf->getOutlineChild(par, i)) {
      return i;
    }
  }
  return -1;
}

QModelIndex OutlineModel::findPageIndex(int pg, QTreeView *tree,
					const QModelIndex &idx) {
  QModelIndex childIdx, lastIdx;
  XpdfOutlineHandle thisItem, childItem;
  int thisPage, lastPage, childPage, i;

  if (idx.isValid()) {
    thisItem = (XpdfOutlineHandle)idx.internalPointer();
    thisPage = pdf->getOutlineTargetPage(thisItem);
    if (thisPage == pg) {
      return idx;
    }
    if (thisPage > pg) {
      return QModelIndex();
    }
    if (!tree->isExpanded(idx)) {
      return idx;
    }
  } else {
    thisPage = 0;
  }

  lastPage = thisPage;
  lastIdx = idx;
  for (i = 0; i < rowCount(idx); ++i) {
    childIdx = findPageIndex(pg, tree, index(i, 0, idx));
    if (!childIdx.isValid()) {
      break;
    }
    childItem = (XpdfOutlineHandle)childIdx.internalPointer();
    childPage = pdf->getOutlineTargetPage(childItem);
    if (childPage > lastPage) {
      lastPage = childPage;
      lastIdx = childIdx;
    }
  }
  if (lastPage == 0) {
    return QModelIndex();
  }
  return lastIdx;
}

void OutlineModel::beginOpenNewDoc() {
  beginResetModel();
}

void OutlineModel::endOpenNewDoc() {
  endResetModel();
}

//------------------------------------------------------------------------
// LayerModel
//------------------------------------------------------------------------

class LayerModel: public QAbstractItemModel {
public:

  LayerModel(XpdfWidget *pdfA);
  virtual QModelIndex index(int row, int column,
			    const QModelIndex &par = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &idx) const;
  virtual int rowCount(const QModelIndex &par = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &par = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &idx, int role) const;
  virtual bool setData(const QModelIndex &idx, const QVariant &value,
		       int role);
  virtual Qt::ItemFlags flags(const QModelIndex &idx) const;

  void beginOpenNewDoc();
  void endOpenNewDoc();

private:

  int getOrderRow(XpdfLayerOrderHandle order) const;

  XpdfWidget *pdf;
};

LayerModel::LayerModel(XpdfWidget *pdfA) {
  pdf = pdfA;
}

QModelIndex LayerModel::index(int row, int column,
			      const QModelIndex &par) const {
  XpdfLayerOrderHandle order;
  int nChildren;

  if (par.isValid()) {
    // children of a layer display order tree node
    order = (XpdfLayerOrderHandle)par.internalPointer();
  } else {
    // children of display order tree root
    order = pdf->getLayerOrderRoot();
  }
  nChildren = pdf->getLayerOrderNumChildren(order);
  if (row < 0 || row >= nChildren || column != 0) {
    return QModelIndex();
  }
  return createIndex(row, 0, pdf->getLayerOrderChild(order, row));
}

QModelIndex LayerModel::parent(const QModelIndex &idx) const {
  XpdfLayerOrderHandle order, par;
  int row;

  if (!idx.isValid()) {
    return QModelIndex();
  }
  order = (XpdfLayerOrderHandle)idx.internalPointer();
  if (!(par = pdf->getLayerOrderParent(order))) {
    return QModelIndex();
  }
  if ((row = getOrderRow(par)) < 0) {
    return QModelIndex();
  }
  return createIndex(row, 0, par);
}

int LayerModel::rowCount(const QModelIndex &par) const {
  XpdfLayerOrderHandle order;

  if (par.isValid()) {
    // children of a layer display order tree node
    order = (XpdfLayerOrderHandle)par.internalPointer();
  } else {
    // children of display order tree root
    order = pdf->getLayerOrderRoot();
  }
  return pdf->getLayerOrderNumChildren(order);
}

int LayerModel::columnCount(const QModelIndex &par) const {
  return 1;
}

QVariant LayerModel::data(const QModelIndex &idx, int role) const {
  XpdfLayerOrderHandle order;
  XpdfLayerHandle layer;

  if (!idx.isValid()) {
    return false;
  }
  if (role == Qt::DisplayRole) {
    order = (XpdfLayerOrderHandle)idx.internalPointer();
    if (pdf->getLayerOrderIsName(order)) {
      return QVariant(pdf->getLayerOrderName(order));
    } else {
      return QVariant(pdf->getLayerName(pdf->getLayerOrderLayer(order)));
    }
  } else if (role == Qt::CheckStateRole) {
    order = (XpdfLayerOrderHandle)idx.internalPointer();
    layer = pdf->getLayerOrderLayer(order);
    if (!layer) {
      return QVariant();
    }
    return pdf->getLayerVisibility(layer) ? Qt::Checked : Qt::Unchecked;
  } else {
    return QVariant();
  }
}

bool LayerModel::setData(const QModelIndex &idx, const QVariant &value,
			 int role) {
  XpdfLayerOrderHandle order;
  XpdfLayerHandle layer;
  bool vis;

  if (!idx.isValid()) {
    return false;
  }
  if (role != Qt::CheckStateRole) {
    return false;
  }
  order = (XpdfLayerOrderHandle)idx.internalPointer();
  layer = pdf->getLayerOrderLayer(order);
  if (!layer) {
    return false;
  }
  vis = value == Qt::Checked;
  if (vis != pdf->getLayerVisibility(layer)) {
    pdf->setLayerVisibility(layer, vis);
  }
  emit dataChanged(idx, idx);
  return true;
}

Qt::ItemFlags LayerModel::flags(const QModelIndex &idx) const {
  Qt::ItemFlags f;

  if (!idx.isValid()) {
    return 0;
  }
  // NB: this does not include Qt::ItemIsUserCheckable because we use
  // the QTreeView::clicked signal to toggle the checkbox -- which
  // handles clicks anywhere in the item, including in the checkbox
  return Qt::ItemIsEnabled;
}

int LayerModel::getOrderRow(XpdfLayerOrderHandle order) const {
  XpdfLayerOrderHandle par;
  int nChildren, i;

  par = pdf->getLayerOrderParent(order);
  if (par) {
    nChildren = pdf->getLayerOrderNumChildren(par);
    for (i = 0; i < nChildren; ++i) {
      if (order == pdf->getLayerOrderChild(par, i)) {
	return i;
      }
    }
    return -1;
  } else {
    return 0;
  }
}

void LayerModel::beginOpenNewDoc() {
  beginResetModel();
}

void LayerModel::endOpenNewDoc() {
  endResetModel();
}

//------------------------------------------------------------------------
// XpdfTabInfo
//------------------------------------------------------------------------

class XpdfTabInfo {
public:

  XpdfTabInfo(QListWidgetItem *listItemA, XpdfWidget *pdfA,
	      QTreeView *outlineTreeA, QTreeView *layerTreeA,
	      QTableWidget *attachmentListA):
    listItem(listItemA), pdf(pdfA), outlineTree(outlineTreeA),
    layerTree(layerTreeA), attachmentList(attachmentListA) {}

  QListWidgetItem *listItem;
  XpdfWidget *pdf;
  QTreeView *outlineTree;
  QTreeView *layerTree;
  QTableWidget *attachmentList;
};

//------------------------------------------------------------------------
// XpdfViewer
//------------------------------------------------------------------------

XpdfViewer::XpdfViewer(XpdfApp *appA, GBool fullScreen) {
  setAttribute(Qt::WA_DeleteOnClose, true);
  app = appA;
  createWindow();
  if (fullScreen) {
    move(0, 0);
    enterFullScreenMode();
  }
}

XpdfViewer *XpdfViewer::create(XpdfApp *app, QString fileName, int page,
			       QString destName, QString password,
			       GBool fullScreen) {
  XpdfViewer *viewer;

  viewer = new XpdfViewer(app, fullScreen);
  if (!viewer->open(fileName, page, destName, password)) {
    viewer->close();
    delete viewer;
    return NULL;
  }
  return viewer;
}

XpdfViewer::~XpdfViewer() {
  destroyWindow();
}

// QSplitter::sizeHint() doesn't take into account collapsed children,
// which leads to an incorrect window size when we start with
// initialSidebarState=no.  So sizeHint() is reimplemented to subtract
// out the sidebar width if needed.
QSize XpdfViewer::sizeHint() const {
  int toolBarWidth, mainWidth, height;

  toolBarWidth = toolBar->sizeHint().width();
  mainWidth = sidebarSplitter->sizeHint().width();
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0 && sizes[1] > 0) {
    mainWidth -= sidebarSplitter->widget(0)->sizeHint().width();
  }
  height = QMainWindow::sizeHint().height();
  return QSize(qMax(toolBarWidth, mainWidth), height);
}

// By default, Qt won't allow windows larger than 2/3 of the screen
// size.  This function kludges around that by resizing to the
// sizeHint, with a max of 60 pixels smaller than the screen size.
void XpdfViewer::tweakSize() {
  QSize hint = sizeHint();
  QRect screen = QApplication::desktop()->availableGeometry();
  int w = hint.width();
  int h = hint.height();
  if (w > screen.width() - 60) {
    w = screen.width() - 60;
  }
  if (h > screen.height() - 60) {
    h = screen.height() - 60;
  }
  resize(w, h);
}

//------------------------------------------------------------------------

GBool XpdfViewer::open(QString fileName, int page, QString destName,
		       QString password) {
  XpdfWidget::ErrorCode err;

  err = currentTab->pdf->loadFile(fileName, password);
  if (err != XpdfWidget::pdfOk) {
    QMessageBox::warning(NULL, "Xpdf Error",
			 "Couldn't open file '" + fileName + "'");
    return gFalse;
  }
  if (!destName.isEmpty()) {
    currentTab->pdf->gotoNamedDestination(destName);
  } else {
    currentTab->pdf->gotoPage(page);
  }
  // after opening a document, focus goes to the XpdfWidget
  currentTab->pdf->setFocus(Qt::OtherFocusReason);
  lastFileOpened = fileName;
  return gTrue;
}

GBool XpdfViewer::openInNewTab(QString fileName, int page, QString destName,
			       QString password, GBool switchToTab) {
  GBool ok;
  int oldTabIndex;

  oldTabIndex = tabList->currentRow();
  addTab();
  updateModeInfo();
  ok = open(fileName, page, destName, password);
  if (!ok) {
    if (tabInfo->getLength() > 1) {
      closeTab(currentTab);
    }
    return gFalse;
  }
  if (!switchToTab) {
    tabList->setCurrentRow(oldTabIndex);
  }
  return gTrue;
}

// Disable the default popup menu (which shows a toggle to hide the
// toolbar).
QMenu *XpdfViewer::createPopupMenu() {
  return NULL;
}

//------------------------------------------------------------------------
// commands
//------------------------------------------------------------------------

void XpdfViewer::execCmd(const char *cmd, QInputEvent *event) {
  GString *name;
  GString *args[cmdMaxArgs];
  const char *p0, *p1;
  int nArgs, i;
  int a, b, m, cmp;

  //----- parse the command
  name = NULL;
  nArgs = 0;
  for (i = 0; i < cmdMaxArgs; ++i) {
    args[i] = NULL;
  }
  p0 = cmd;
  for (p1 = p0; *p1 && isalnum(*p1); ++p1) ;
  if (p1 == p0) {
    goto err1;
  }
  name = new GString(p0, p1 - p0);
  if (*p1 == '(') {
    while (nArgs < cmdMaxArgs) {
      p0 = p1 + 1;
      for (p1 = p0; *p1 && *p1 != ',' && *p1 != ')'; ++p1) ;
      args[nArgs++] = new GString(p0, p1 - p0);
      if (*p1 != ',') {
	break;
      }
    }
    if (*p1 != ')') {
      goto err1;
    }
    ++p1;
  }
  if (*p1) {
    goto err1;
  }
  
  //----- find the command
  a = -1;
  b = nCmds;
  // invariant: cmdTab[a].name < name < cmdTab[b].name
  while (b - a > 1) {
    m = (a + b) / 2;
    cmp = strcmp(cmdTab[m].name, name->getCString());
    if (cmp < 0) {
      a = m;
    } else if (cmp > 0) {
      b = m;
    } else {
      a = b = m;
    }
  }
  if (cmp != 0) {
    goto err1;
  }

  //----- execute the command
  if (nArgs != cmdTab[a].nArgs ||
      (cmdTab[a].requiresEvent && !event)) {
    goto err1;
  }
  if (cmdTab[a].requiresDoc && !currentTab->pdf->hasOpenDocument()) {
    // don't issue an error message for this -- it happens, e.g., when
    // clicking in a window with no open PDF file
    goto err2;
  }
  (this->*cmdTab[a].func)(args, nArgs, event);

  //----- clean up
  delete name;
  for (i = 0; i < nArgs; ++i) {
    if (args[i]) {
      delete args[i];
    }
  }
  return;

 err1:
  error(errConfig, -1, "Invalid command syntax: '{0:s}'", cmd);
 err2:
  if (name) {
    delete name;
  }
  for (i = 0; i < nArgs; ++i) {
    if (args[i]) {
      delete args[i];
    }
  }
}

static int mouseX(QInputEvent *event) {
  QEvent::Type eventType;

  if (!event) {
    return 0;
  }
  eventType = event->type();
  if (eventType == QEvent::MouseButtonPress ||
      eventType == QEvent::MouseButtonRelease ||
      eventType == QEvent::MouseButtonDblClick ||
      eventType == QEvent::MouseMove) {
    return ((QMouseEvent *)event)->x();
  } else if (eventType == QEvent::Wheel) {
    return ((QWheelEvent *)event)->x();
  } else {
    return 0;
  }
}

static int mouseY(QInputEvent *event) {
  QEvent::Type eventType;

  if (!event) {
    return 0;
  }
  eventType = event->type();
  if (eventType == QEvent::MouseButtonPress ||
      eventType == QEvent::MouseButtonRelease ||
      eventType == QEvent::MouseButtonDblClick ||
      eventType == QEvent::MouseMove) {
    return ((QMouseEvent *)event)->y();
  } else if (eventType == QEvent::Wheel) {
    return ((QWheelEvent *)event)->y();
  } else {
    return 0;
  }
}

void XpdfViewer::cmdAbout(GString *args[], int nArgs, QInputEvent *event) {
  if (!aboutDialog) {
    createAboutDialog();
  }
  aboutDialog->show();
  aboutDialog->raise();
}

void XpdfViewer::cmdBlockSelectMode(GString *args[], int nArgs,
				    QInputEvent *event) {
  currentTab->pdf->setBlockSelectMode();
  updateSelectModeInfo();
}

void XpdfViewer::cmdCloseSidebar(GString *args[], int nArgs,
				 QInputEvent *event) {
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    return;
  }
  sidebarWidth = sizes[0];
  sizes[0] = 0;
  sizes[1] += sidebarWidth;
  sidebarSplitter->setSizes(sizes);
}

void XpdfViewer::cmdCloseSidebarMoveResizeWin(GString *args[], int nArgs,
					      QInputEvent *event) {
  int newWidth;
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    return;
  }
  sidebarWidth = sizes[0];
  newWidth = width() - sidebarWidth;
  sizes[0] = 0;
  sidebarSplitter->setSizes(sizes);
  setGeometry(geometry().x() + sidebarWidth, geometry().y(),
	      newWidth, height());
}

void XpdfViewer::cmdCloseSidebarResizeWin(GString *args[], int nArgs,
					  QInputEvent *event) {
  int newWidth;
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    return;
  }
  sidebarWidth = sizes[0];
  newWidth = width() - sidebarWidth;
  sizes[0] = 0;
  sidebarSplitter->setSizes(sizes);
  resize(newWidth, height());
}

void XpdfViewer::cmdCloseTabOrQuit(GString *args[], int nArgs,
				   QInputEvent *event) {
  closeTab(currentTab);
  if (tabInfo->getLength() == 0) {
    app->closeWindowOrQuit(this);
    return;
  }
}

void XpdfViewer::cmdCloseWindowOrQuit(GString *args[], int nArgs,
				      QInputEvent *event) {
  app->closeWindowOrQuit(this);
}


void XpdfViewer::cmdContinuousMode(GString *args[], int nArgs,
				   QInputEvent *event) {
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplayContinuous);
  updateModeInfo();
}

void XpdfViewer::cmdCopy(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->copySelection();
}

#if 0 // for debugging
void XpdfViewer::cmdDebug1(GString *args[], int nArgs, QInputEvent *event) {
}
#endif

void XpdfViewer::cmdEndPan(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->getCore()->endPan(mouseX(event), mouseY(event));
}

void XpdfViewer::cmdEndSelection(GString *args[], int nArgs,
				 QInputEvent *event) {
  currentTab->pdf->getCore()->endSelection(mouseX(event), mouseY(event));
}

void XpdfViewer::cmdFind(GString *args[], int nArgs, QInputEvent *event) {
  clearFindError();
  findEdit->setFocus(Qt::OtherFocusReason);
  findEdit->selectAll();
}

void XpdfViewer::cmdFindFirst(GString *args[], int nArgs, QInputEvent *event) {
  int flags;

  clearFindError();
  flags = 0;
  if (findCaseSensitiveAction->isChecked()) {
    flags |= XpdfWidget::findCaseSensitive;
  }
  if (findWholeWordsAction->isChecked()) {
    flags |= XpdfWidget::findWholeWord;
  }
  if (!currentTab->pdf->find(findEdit->text(), flags)) {
    showFindError();
  }
}

void XpdfViewer::cmdFindNext(GString *args[], int nArgs, QInputEvent *event) {
  int flags;

  clearFindError();
  flags = XpdfWidget::findNext;
  if (findCaseSensitiveAction->isChecked()) {
    flags |= XpdfWidget::findCaseSensitive;
  }
  if (findWholeWordsAction->isChecked()) {
    flags |= XpdfWidget::findWholeWord;
  }
  if (!currentTab->pdf->find(findEdit->text(), flags)) {
    showFindError();
  }
}

void XpdfViewer::cmdFindPrevious(GString *args[], int nArgs,
				 QInputEvent *event) {
  int flags;

  clearFindError();
  flags = XpdfWidget::findBackward | XpdfWidget::findNext;
  if (findCaseSensitiveAction->isChecked()) {
    flags |= XpdfWidget::findCaseSensitive;
  }
  if (findWholeWordsAction->isChecked()) {
    flags |= XpdfWidget::findWholeWord;
  }
  if (!currentTab->pdf->find(findEdit->text(), flags)) {
    showFindError();
  }
}

void XpdfViewer::cmdFocusToDocWin(GString *args[], int nArgs,
				  QInputEvent *event) {
  currentTab->pdf->setFocus(Qt::OtherFocusReason);
}

void XpdfViewer::cmdFocusToPageNum(GString *args[], int nArgs,
				   QInputEvent *event) {
  pageNumber->setFocus(Qt::OtherFocusReason);
  pageNumber->selectAll();
}

void XpdfViewer::cmdFollowLink(GString *args[], int nArgs,
			       QInputEvent *event) {
  followLink(event, gFalse, gFalse, gFalse);
}

void XpdfViewer::cmdFollowLinkInNewTab(GString *args[], int nArgs,
				       QInputEvent *event) {
  followLink(event, gFalse, gTrue, gFalse);
}

void XpdfViewer::cmdFollowLinkInNewTabNoSel(GString *args[], int nArgs,
					    QInputEvent *event) {
  followLink(event, gTrue, gTrue, gFalse);
}

void XpdfViewer::cmdFollowLinkInNewWin(GString *args[], int nArgs,
				       QInputEvent *event) {
  followLink(event, gFalse, gFalse, gTrue);
}

void XpdfViewer::cmdFollowLinkInNewWinNoSel(GString *args[], int nArgs,
					    QInputEvent *event) {
  followLink(event, gTrue, gFalse, gTrue);
}

void XpdfViewer::cmdFollowLinkNoSel(GString *args[], int nArgs,
				    QInputEvent *event) {
  followLink(event, gTrue, gFalse, gFalse);
}

void XpdfViewer::cmdFullScreenMode(GString *args[], int nArgs,
				   QInputEvent *event) {
  if (!(windowState() & Qt::WindowFullScreen)) {
    enterFullScreenMode();
  }
}

void XpdfViewer::cmdGotoDest(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->gotoNamedDestination(args[0]->getCString());
  updateZoomInfo();
}

void XpdfViewer::cmdGotoLastPage(GString *args[], int nArgs,
				 QInputEvent *event) {
  currentTab->pdf->gotoLastPage();
}

void XpdfViewer::cmdGoBackward(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->goBackward();
}

void XpdfViewer::cmdGoForward(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->goForward();
}

void XpdfViewer::cmdGotoPage(GString *args[], int nArgs, QInputEvent *event) {
  int pg;

  pg = atoi(args[0]->getCString());
  if (pg < 1 || pg > currentTab->pdf->getNumPages()) {
    return;
  }
  currentTab->pdf->gotoPage(pg);
}

void XpdfViewer::cmdHelp(GString *args[], int nArgs, QInputEvent *event) {
  QDesktopServices::openUrl(QUrl(helpURL, QUrl::TolerantMode));
}

void XpdfViewer::cmdHorizontalContinuousMode(GString *args[], int nArgs,
					     QInputEvent *event) {
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplayHorizontalContinuous);
}

void XpdfViewer::cmdLinearSelectMode(GString *args[], int nArgs,
				     QInputEvent *event) {
  currentTab->pdf->setLinearSelectMode();
  updateSelectModeInfo();
}

void XpdfViewer::cmdNewTab(GString *args[], int nArgs, QInputEvent *event) {
  addTab();
  updateModeInfo();
  updateDocInfo();
}

void XpdfViewer::cmdNewWindow(GString *args[], int nArgs, QInputEvent *event) {
  app->newWindow();
}

void XpdfViewer::cmdNextPage(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->gotoNextPage();
}

void XpdfViewer::cmdNextPageNoScroll(GString *args[], int nArgs,
				     QInputEvent *event) {
  currentTab->pdf->gotoNextPage(false);
}

void XpdfViewer::cmdNextTab(GString *args[], int nArgs, QInputEvent *event) {
  int i;

  if (tabInfo->getLength() == 1) {
    return;
  }
  for (i = 0; i < tabInfo->getLength(); ++i) {
    if ((XpdfTabInfo *)tabInfo->get(i) == currentTab) {
      ++i;
      if (i >= tabInfo->getLength()) {
	i = 0;
      }
      tabList->setCurrentRow(i);
      return;
    }
  }
}

void XpdfViewer::cmdOpen(GString *args[], int nArgs,
			 QInputEvent *event) {
  QString startFile, fileName;
  QDir startDir;

  if (!(startFile = currentTab->pdf->getFileName()).isEmpty()) {
    startDir = QDir(startFile);
    startDir.cdUp();
  } else if (!lastFileOpened.isEmpty()) {
    startDir = QDir(lastFileOpened);
    startDir.cdUp();
  } else {
    startDir = QDir(".");
  }
  fileName = QFileDialog::getOpenFileName(this, "Open PDF File",
					  startDir.canonicalPath(),
					  "PDF files (*.pdf)");
  if (fileName.isEmpty()) {
    return;
  }
  open(fileName, 1, "", "");
}

void XpdfViewer::cmdOpenErrorWindow(GString *args[], int nArgs,
				    QInputEvent *event) {
  errorWindow->show();
  errorWindow->raise();
}

void XpdfViewer::cmdOpenSidebar(GString *args[], int nArgs,
				QInputEvent *event) {
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] > 0) {
    return;
  }
  sizes[0] = sidebarWidth;
  sizes[1] -= sidebarWidth;
  sidebarSplitter->setSizes(sizes);
}

void XpdfViewer::cmdOpenSidebarMoveResizeWin(GString *args[], int nArgs,
					     QInputEvent *event) {
  int newWidth;
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] > 0) {
    return;
  }
  sizes[0] = sidebarWidth;
  newWidth = width() + sidebarWidth;
  sidebarSplitter->setSizes(sizes);
  setGeometry(geometry().x() - sidebarWidth, geometry().y(),
	      newWidth, height());
}

void XpdfViewer::cmdOpenSidebarResizeWin(GString *args[], int nArgs,
					 QInputEvent *event) {
  int newWidth;
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] > 0) {
    return;
  }
  sizes[0] = sidebarWidth;
  newWidth = width() + sidebarWidth;
  sidebarSplitter->setSizes(sizes);
  resize(newWidth, height());
}

void XpdfViewer::cmdPageDown(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->scrollPageDown();
}

void XpdfViewer::cmdPageUp(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->scrollPageUp();
}

void XpdfViewer::cmdPostPopupMenu(GString *args[], int nArgs,
				  QInputEvent *event) {
  if (!popupMenu) {
    return;
  }
  popupMenu->exec(currentTab->pdf->mapToGlobal(QPoint(mouseX(event),
						      mouseY(event))));
}

void XpdfViewer::cmdPrevPage(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->gotoPreviousPage();
}

void XpdfViewer::cmdPrevPageNoScroll(GString *args[], int nArgs,
				     QInputEvent *event) {
  currentTab->pdf->gotoPreviousPage(false);
}

void XpdfViewer::cmdPrevTab(GString *args[], int nArgs, QInputEvent *event) {
  int i;

  if (tabInfo->getLength() == 1) {
    return;
  }
  for (i = 0; i < tabInfo->getLength(); ++i) {
    if ((XpdfTabInfo *)tabInfo->get(i) == currentTab) {
      --i;
      if (i < 0) {
	i = tabInfo->getLength() - 1;
      }
      tabList->setCurrentRow(i);
      return;
    }
  }
}


#if XPDFWIDGET_PRINTING
void XpdfViewer::cmdPrint(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->print(true);
}
#endif

void XpdfViewer::cmdQuit(GString *args[], int nArgs,
			 QInputEvent *event) {
  app->quit();
}

void XpdfViewer::cmdReload(GString *args[], int nArgs, QInputEvent *event) {
  if (currentTab->pdf->reload() != XpdfWidget::pdfOk) {
    QMessageBox::warning(NULL, "Xpdf Error", "Couldn't reload file");
  }
}

void XpdfViewer::cmdRotateCW(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->setRotate((currentTab->pdf->getRotate() + 90) % 360);
}

void XpdfViewer::cmdRotateCCW(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->setRotate((currentTab->pdf->getRotate() + 270) % 360);
}

static QString mungeURL(QString url) {
  static const char *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789"
                               "-_.~/?:@&=+,#%";
  QString newURL;
  char c;
  char buf[4];
  int i;

  for (i = 0; i < url.length(); ++i) {
    c = url.at(i).toLatin1();
    if (strchr(allowed, c)) {
      newURL += c;
    } else {
      sprintf(buf, "%%%02x", c & 0xff);
      newURL += buf;
    }
  }
  return newURL;
}

void XpdfViewer::cmdRun(GString *args[], int nArgs, QInputEvent *event) {
  GString *fmt, *cmd;
  QString s;
  QPoint pt;
  double selLRX, selLRY, selURX, selURY, mX, mY;
  int selPage, mPage;
  GBool gotSel, gotMouse;
  char buf[64];
  const char *p, *q;
  char c0, c1;
  int i;

  cmd = new GString();
  fmt = args[0];
  i = 0;
  gotSel = gotMouse = gFalse;
  while (i < fmt->getLength()) {
    c0 = fmt->getChar(i);
    if (c0 == '%' && i+1 < fmt->getLength()) {
      c1 = fmt->getChar(i+1);
      switch (c1) {
      case 'f':
	cmd->append(currentTab->pdf->getFileName().toLocal8Bit().constData());
	break;
      case 'b':
	p = currentTab->pdf->getFileName().toLocal8Bit().constData();
	if ((q = strrchr(p, '.'))) {
	  cmd->append(p, q - p);
	} else {
	  cmd->append(p);
	}
	break;
      case 'u':
	s = currentTab->pdf->getMouseLinkInfo();
	s = mungeURL(s);
	cmd->append(s.toLocal8Bit().constData());
	break;
      case 'p':
	sprintf(buf, "%d", currentTab->pdf->getMidPage());
	cmd->append(buf);
	break;
      case 'x':
      case 'y':
      case 'X':
      case 'Y':
	if (!gotSel) {
	  if (!currentTab->pdf->getCurrentSelection(&selPage, &selURX, &selURY,
						    &selLRX, &selLRY)) {
	    selPage = 0;
	    selURX = selURY = selLRX = selLRY = 0;
	  }
	  gotSel = gTrue;
	}
	sprintf(buf, "%g",
		(c1 == 'x') ? selURX :
		(c1 == 'y') ? selURY :
		(c1 == 'X') ? selLRX : selLRY);
	cmd->append(buf);
	break;
      case 'i':
      case 'j':
      case 'k':
	if (!gotMouse) {
	  if (event->type() == QEvent::MouseButtonPress ||
	      event->type() == QEvent::MouseButtonRelease ||
	      event->type() == QEvent::MouseButtonDblClick ||
	      event->type() == QEvent::MouseMove ||
	      event->type() == QEvent::Wheel) {
	    currentTab->pdf->convertWindowToPDFCoords(mouseX(event),
						      mouseY(event),
						      &mPage, &mX, &mY);
	  } else {
	    pt = currentTab->pdf->mapFromGlobal(QCursor::pos());
	    currentTab->pdf->convertWindowToPDFCoords(pt.x(), pt.y(),
						      &mPage, &mX, &mY);
	  }
	  gotMouse = gTrue;
	}
	if (c1 == 'i') {
	  sprintf(buf, "%d", mPage);
	} else {
	  sprintf(buf, "%g", (c1 == 'j') ? mX : mY);
	}
	cmd->append(buf);
	break;
      default:
	cmd->append(c1);
	break;
      }
      i += 2;
    } else {
      cmd->append(c0);
      ++i;
    }
  }

  QProcess::startDetached(cmd->getCString());
  delete cmd;
}

void XpdfViewer::cmdSaveAs(GString *args[], int nArgs, QInputEvent *event) {
  QString startFile, fileName;
  QDir startDir;

  if (!(startFile = currentTab->pdf->getFileName()).isEmpty()) {
    startDir = QDir(startFile);
    startDir.cdUp();
  } else {
    startDir = QDir(".");
  }
  fileName = QFileDialog::getSaveFileName(this, "Save PDF File",
					  startDir.canonicalPath(),
					  "PDF files (*.pdf)");
  if (fileName.isEmpty()) {
    return;
  }
  currentTab->pdf->saveAs(fileName);
}

void XpdfViewer::cmdSaveImage(GString *args[], int nArgs, QInputEvent *event) {
  execSaveImageDialog();
}

void XpdfViewer::cmdScrollDown(GString *args[], int nArgs,
			       QInputEvent *event) {
  int dy = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->scrollBy(0, dy);
}

void XpdfViewer::cmdScrollDownNextPage(GString *args[], int nArgs,
				       QInputEvent *event) {
  int dy = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->getCore()->scrollDownNextPage(dy);
}

void XpdfViewer::cmdScrollLeft(GString *args[], int nArgs,
			       QInputEvent *event) {
  int dx = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->scrollBy(-dx, 0);
}

void XpdfViewer::cmdScrollOutlineDown(GString *args[], int nArgs,
				      QInputEvent *event) {
  QScrollBar *sb = currentTab->outlineTree->verticalScrollBar();
  sb->setValue(sb->value() + atoi(args[0]->getCString()));
}

void XpdfViewer::cmdScrollOutlineUp(GString *args[], int nArgs,
				    QInputEvent *event) {
  QScrollBar *sb = currentTab->outlineTree->verticalScrollBar();
  sb->setValue(sb->value() - atoi(args[0]->getCString()));
}

void XpdfViewer::cmdScrollRight(GString *args[], int nArgs,
				QInputEvent *event) {
  int dx = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->scrollBy(dx, 0);
}

void XpdfViewer::cmdScrollToBottomEdge(GString *args[], int nArgs,
				       QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToBottomEdge();
}

void XpdfViewer::cmdScrollToBottomRight(GString *args[], int nArgs,
					QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToBottomRight();
}

void XpdfViewer::cmdScrollToLeftEdge(GString *args[], int nArgs,
				     QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToLeftEdge();
}

void XpdfViewer::cmdScrollToRightEdge(GString *args[], int nArgs,
				      QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToRightEdge();
}

void XpdfViewer::cmdScrollToTopEdge(GString *args[], int nArgs,
				    QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToTopEdge();
}

void XpdfViewer::cmdScrollToTopLeft(GString *args[], int nArgs,
				    QInputEvent *event) {
  currentTab->pdf->getCore()->scrollToTopLeft();
}

void XpdfViewer::cmdScrollUp(GString *args[], int nArgs,
			     QInputEvent *event) {
  int dy = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->scrollBy(0, -dy);
}

void XpdfViewer::cmdScrollUpPrevPage(GString *args[], int nArgs,
				     QInputEvent *event) {
  int dy = scaleScroll(atoi(args[0]->getCString()));
  currentTab->pdf->getCore()->scrollUpPrevPage(dy);
}

void XpdfViewer::cmdSetSelection(GString *args[], int nArgs,
				 QInputEvent *event) {
  currentTab->pdf->setCurrentSelection(atoi(args[0]->getCString()),
				       atof(args[1]->getCString()),
				       atof(args[2]->getCString()),
				       atof(args[3]->getCString()),
				       atof(args[4]->getCString()));
}

void XpdfViewer::cmdSideBySideContinuousMode(GString *args[], int nArgs,
					     QInputEvent *event) {
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplaySideBySideContinuous);
}

void XpdfViewer::cmdSideBySideSingleMode(GString *args[], int nArgs,
					 QInputEvent *event) {
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplaySideBySideSingle);
}

void XpdfViewer::cmdSinglePageMode(GString *args[], int nArgs,
				   QInputEvent *event) {
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplaySingle);
  updateModeInfo();
}

void XpdfViewer::cmdStartPan(GString *args[], int nArgs, QInputEvent *event) {
  currentTab->pdf->getCore()->startPan(mouseX(event), mouseY(event));
}

void XpdfViewer::cmdStartSelection(GString *args[], int nArgs,
				   QInputEvent *event) {
  currentTab->pdf->getCore()->startSelection(mouseX(event), mouseY(event));
}

void XpdfViewer::cmdToggleContinuousMode(GString *args[], int nArgs,
					 QInputEvent *event) {
  XpdfWidget::DisplayMode mode = currentTab->pdf->getDisplayMode();
  if (mode == XpdfWidget::pdfDisplaySingle) {
    currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplayContinuous);
  } else {
    currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplaySingle);
  }
  updateModeInfo();
}

void XpdfViewer::cmdToggleFullScreenMode(GString *args[], int nArgs,
					 QInputEvent *event) {
  if (windowState() & Qt::WindowFullScreen) {
    exitFullScreenMode();
  } else {
    enterFullScreenMode();
  }
}

void XpdfViewer::cmdToggleSelectMode(GString *args[], int nArgs,
				     QInputEvent *event) {
  if (currentTab->pdf->isBlockSelectMode()) {
    currentTab->pdf->setLinearSelectMode();
  } else {
    currentTab->pdf->setBlockSelectMode();
  }
  updateSelectModeInfo();
}

void XpdfViewer::cmdToggleSidebar(GString *args[], int nArgs,
				  QInputEvent *event) {
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    cmdOpenSidebar(args, nArgs, event);
  } else {
    cmdCloseSidebar(args, nArgs, event);
  }
}

void XpdfViewer::cmdToggleSidebarMoveResizeWin(GString *args[], int nArgs,
					       QInputEvent *event) {
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    cmdOpenSidebarMoveResizeWin(args, nArgs, event);
  } else {
    cmdCloseSidebarMoveResizeWin(args, nArgs, event);
  }
}

void XpdfViewer::cmdToggleSidebarResizeWin(GString *args[], int nArgs,
					   QInputEvent *event) {
  QList<int> sizes = sidebarSplitter->sizes();
  if (sizes[0] == 0) {
    cmdOpenSidebarResizeWin(args, nArgs, event);
  } else {
    cmdCloseSidebarResizeWin(args, nArgs, event);
  }
}

void XpdfViewer::cmdWindowMode(GString *args[], int nArgs,
			       QInputEvent *event) {
  if (windowState() & Qt::WindowFullScreen) {
    exitFullScreenMode();
  }
}

void XpdfViewer::cmdZoomFitPage(GString *args[], int nArgs,
				QInputEvent *event) {
  currentTab->pdf->zoomCentered(XpdfWidget::zoomToPage);
  updateZoomInfo();
}

void XpdfViewer::cmdZoomFitWidth(GString *args[], int nArgs,
				 QInputEvent *event) {
  currentTab->pdf->zoomCentered(XpdfWidget::zoomToWidth);
  updateZoomInfo();
}

void XpdfViewer::cmdZoomIn(GString *args[], int nArgs, QInputEvent *event) {
  double z;
  int i;

  z = currentTab->pdf->getZoomPercent(currentTab->pdf->getMidPage());
  for (i = 0; i < zoomComboBox->count(); ++i) {
    if (zoomComboBoxVals[i] > z) {
      currentTab->pdf->zoomCentered(zoomComboBoxVals[i]);
      zoomComboBox->setCurrentIndex(i);
      updateZoomInfo();
      break;
    }
  }
}

void XpdfViewer::cmdZoomOut(GString *args[], int nArgs, QInputEvent *event) {
  double z;
  int i;

  z = currentTab->pdf->getZoomPercent(currentTab->pdf->getMidPage());
  for (i = zoomComboBox->count() - 1; i >= 0; --i) {
    if (zoomComboBoxVals[i] < z) {
      currentTab->pdf->zoomCentered(zoomComboBoxVals[i]);
      zoomComboBox->setCurrentIndex(i);
      updateZoomInfo();
      break;
    }
  }
}

void XpdfViewer::cmdZoomPercent(GString *args[], int nArgs,
				QInputEvent *event) {
  QString zoomStr;
  int z;

  z = (int)floor(atof(args[0]->getCString()) + 0.5);
  if (z > maxZoom) {
    z = maxZoom;
  }
  currentTab->pdf->zoomCentered(z);
  updateZoomInfo();
}

void XpdfViewer::cmdZoomToSelection(GString *args[], int nArgs,
				    QInputEvent *event) {
  double x0, y0, x1, y1, rx, ry, z, w, h, t;
  int pg, xx0, yy0, xx1, yy1;

  if (currentTab->pdf->getCurrentSelection(&pg, &x0, &y0, &x1, &y1)) {
    z = currentTab->pdf->getZoomPercent(pg);
    currentTab->pdf->getCore()->cvtUserToDev(pg, x0, y0, &xx0, &yy0);
    currentTab->pdf->getCore()->cvtUserToDev(pg, x1, y1, &xx1, &yy1);
    rx = (double)currentTab->pdf->getCore()->getWindowWidth()
         / (double)abs(xx1 - xx0);
    ry = (double)currentTab->pdf->getCore()->getWindowHeight()
         / (double)abs(yy1 - yy0);
    z *= rx < ry ? rx : ry;
    if (z > maxZoom) {
      w = 0.5 * (z / maxZoom) * (x1 - x0);
      h = 0.5 * (z / maxZoom) * (y1 - y0);
      t = 0.5 * (x0 + x1);
      x0 = t - w;
      x1 = t + w;
      t = 0.5 * (y0 + y1);
      y0 = t - h;
      y1 = t + h;
    }
    currentTab->pdf->zoomToRect(pg, x0, y0, x1, y1);
    updateZoomInfo();
  }
}

int XpdfViewer::scaleScroll(int delta) {
  int scaledDelta;

  scaledDelta = (delta * currentTab->pdf->getCore()->getDisplayDpi()) / 96;
  if (delta >= 0) {
    if (scaledDelta < delta) {
      scaledDelta = delta;
    }
  } else {
    if (scaledDelta > delta) {
      scaledDelta = delta;
    }
  }
  return scaledDelta;
}

void XpdfViewer::followLink(QInputEvent *event, GBool onlyIfNoSel,
			    GBool newTab, GBool newWindow) {
  int pg, targetPage;
  double x, y;
  QString targetFileName, targetDest;

  if (onlyIfNoSel && currentTab->pdf->hasSelection()) {
    return;
  }
  if (!currentTab->pdf->convertWindowToPDFCoords(mouseX(event), mouseY(event),
						 &pg, &x, &y)) {
    return;
  }
  if ((newTab || newWindow) &&
      currentTab->pdf->getLinkTarget(pg, x, y, targetFileName,
				     targetPage, targetDest)) {
    if (newTab) {
      openInNewTab(targetFileName, targetPage, targetDest, "", gTrue);
    } else {
      app->openInNewWindow(targetFileName, targetPage, targetDest, "");
    }
  } else {
    if (!currentTab->pdf->gotoLinkAt(pg, x, y)) {
      QMessageBox::warning(NULL,
			   "Xpdf Error",
			   "Couldn't follow link: '"
			   + currentTab->pdf->getLinkInfo(pg, x, y)
			   + "'");
    }
  }
}

//------------------------------------------------------------------------
// GUI events
//------------------------------------------------------------------------

void XpdfViewer::pdfResized() {
  updateZoomInfo();
}

void XpdfViewer::pdfPaintDone(bool finished) {
  if (finished) {
    statusIndicatorStop();
  } else {
    statusIndicatorStart();
  }
}

void XpdfViewer::preLoad() {
  ((OutlineModel *)currentTab->outlineTree->model())->beginOpenNewDoc();
  ((LayerModel *)currentTab->layerTree->model())->beginOpenNewDoc();
}

void XpdfViewer::postLoad() {
  ((LayerModel *)currentTab->layerTree->model())->endOpenNewDoc();
  ((OutlineModel *)currentTab->outlineTree->model())->endOpenNewDoc();
  setOutlineOpenItems(QModelIndex());
  fillAttachmentList();
  updateDocInfo();
}

void XpdfViewer::keyPress(QKeyEvent *e) {
  GList *cmds;
  int mods, qtKey, keyCode, i;

  mods = getModifiers(e->modifiers());

  qtKey = e->key();
  if (qtKey >= 0x20 && qtKey <= 0xfe) {
    keyCode = qtKey;
    if (keyCode >= 'A' && keyCode <= 'Z' && !(mods & xpdfKeyModShift)) {
      keyCode += 'a' - 'A';
    }
  } else if (qtKey == Qt::Key_Tab || qtKey == Qt::Key_Backtab) {
    // backtab = shift-tab
    keyCode = xpdfKeyCodeTab;
  } else if (qtKey == Qt::Key_Return) {
    keyCode = xpdfKeyCodeReturn;
  } else if (qtKey == Qt::Key_Enter) {
    keyCode = xpdfKeyCodeEnter;
  } else if (qtKey == Qt::Key_Backspace) {
    keyCode = xpdfKeyCodeBackspace;
  } else if (qtKey == Qt::Key_Insert) {
    keyCode = xpdfKeyCodeInsert;
  } else if (qtKey == Qt::Key_Delete) {
    keyCode = xpdfKeyCodeDelete;
  } else if (qtKey == Qt::Key_Home) {
    keyCode = xpdfKeyCodeHome;
  } else if (qtKey == Qt::Key_End) {
    keyCode = xpdfKeyCodeEnd;
  } else if (qtKey == Qt::Key_PageUp) {
    keyCode = xpdfKeyCodePgUp;
  } else if (qtKey == Qt::Key_PageDown) {
    keyCode = xpdfKeyCodePgDn;
  } else if (qtKey == Qt::Key_Left) {
    keyCode = xpdfKeyCodeLeft;
  } else if (qtKey == Qt::Key_Right) {
    keyCode = xpdfKeyCodeRight;
  } else if (qtKey == Qt::Key_Up) {
    keyCode = xpdfKeyCodeUp;
  } else if (qtKey == Qt::Key_Down) {
    keyCode = xpdfKeyCodeDown;
  } else if (qtKey == Qt::Key_Escape) {
    keyCode = xpdfKeyCodeEsc;
  } else if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F35) {
    keyCode = xpdfKeyCodeF1 + (qtKey - Qt::Key_F1);
  } else {
    return;
  }

  if ((cmds = globalParams->getKeyBinding(keyCode, mods,
					  getContext(e->modifiers())))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      execCmd(((GString *)cmds->get(i))->getCString(), e);
    }
    deleteGList(cmds, GString);
  }
}

void XpdfViewer::mousePress(QMouseEvent *e) {
  GList *cmds;
  int btn, keyCode, i;

  if (!(btn = getMouseButton(e->button()))) {
    return;
  }
  lastMousePressX = e->x();
  lastMousePressY = e->y();
  keyCode = xpdfKeyCodeMousePress1 + (btn - 1);
  if ((cmds = globalParams->getKeyBinding(keyCode,
					  getModifiers(e->modifiers()),
					  getContext(e->modifiers())))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      execCmd(((GString *)cmds->get(i))->getCString(), e);
    }
    deleteGList(cmds, GString);
  }
}

void XpdfViewer::mouseRelease(QMouseEvent *e) {
  GList *cmds;
  int btn, keyCode, i;

  if (!(btn = getMouseButton(e->button()))) {
    return;
  }
  keyCode = xpdfKeyCodeMouseRelease1 + (btn - 1);
  if ((cmds = globalParams->getKeyBinding(keyCode,
					  getModifiers(e->modifiers()),
					  getContext(e->modifiers())))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      execCmd(((GString *)cmds->get(i))->getCString(), e);
    }
    deleteGList(cmds, GString);
  }
  if (abs(e->x() - lastMousePressX)
      + abs(e->y() - lastMousePressY) <= QApplication::startDragDistance()) {
    keyCode = xpdfKeyCodeMouseClick1 + (btn - 1);
    if ((cmds = globalParams->getKeyBinding(keyCode,
					    getModifiers(e->modifiers()),
					    getContext(e->modifiers())))) {
      for (i = 0; i < cmds->getLength(); ++i) {
	execCmd(((GString *)cmds->get(i))->getCString(), e);
      }
      deleteGList(cmds, GString);
    }
  }
}

void XpdfViewer::mouseWheel(QWheelEvent *e) {
  GList *cmds;
  int keyCode, i;

  // for historical reasons xpdf uses X11 button numbering for mouse
  // wheel events
  if (e->orientation() == Qt::Vertical) {
    if (e->delta() > 0) {
      keyCode = xpdfKeyCodeMousePress4;
    } else {
      keyCode = xpdfKeyCodeMousePress5;
    }
  } else {
    if (e->delta() > 0) {
      keyCode = xpdfKeyCodeMousePress6;
    } else {
      keyCode = xpdfKeyCodeMousePress7;
    }
  }
  if ((cmds = globalParams->getKeyBinding(keyCode,
					  getModifiers(e->modifiers()),
					  getContext(e->modifiers())))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      execCmd(((GString *)cmds->get(i))->getCString(), e);
    }
    deleteGList(cmds, GString);
  }
}

void XpdfViewer::mouseMove(QMouseEvent *e) {
  int pg;
  double x, y;
  QString info;

  currentTab->pdf->convertWindowToPDFCoords(e->x(), e->y(), &pg, &x, &y);
  info = currentTab->pdf->getLinkInfo(pg, x, y);
  if (info == linkTargetInfo) {
    return;
  }
  linkTargetInfo = info;
  if (linkTargetInfo.isEmpty()) {
    currentTab->pdf->unsetCursor();
    linkTargetBar->hide();
  } else {
    currentTab->pdf->setCursor(Qt::PointingHandCursor);
    linkTargetBar->setText(linkTargetInfo);
    linkTargetBar->resize(linkTargetBar->sizeHint());
    if (e->x() > viewerStack->width() / 2) {
      x = viewerStack->x();
    } else {
      x = width() - linkTargetBar->width();
      if (x < viewerStack->x()) {
	x = viewerStack->x();
      }
    }
    linkTargetBar->move(x, height() - linkTargetBar->height());
    linkTargetBar->show();
  }
}

int XpdfViewer::getModifiers(Qt::KeyboardModifiers qtMods) {
  int mods;

  mods = 0;
  if (qtMods & Qt::ShiftModifier) {
    mods |= xpdfKeyModShift;
  }
  if (qtMods & Qt::ControlModifier) {
    mods |= xpdfKeyModCtrl;
  }
  if (qtMods & Qt::AltModifier) {
    mods |= xpdfKeyModAlt;
  }
  return mods;
}

int XpdfViewer::getContext(Qt::KeyboardModifiers qtMods) {
  XpdfWidget::DisplayMode mode;
  GBool contin;
  int context;

  mode = currentTab->pdf->getDisplayMode();
  contin = mode == XpdfWidget::pdfDisplayContinuous ||
           mode == XpdfWidget::pdfDisplaySideBySideContinuous ||
           mode == XpdfWidget::pdfDisplayHorizontalContinuous;
  context = ((windowState() & Qt::WindowFullScreen) ? xpdfKeyContextFullScreen
                                                    : xpdfKeyContextWindow) |
            (contin ? xpdfKeyContextContinuous
	            : xpdfKeyContextSinglePage) |
            (currentTab->pdf->mouseOverLink() ? xpdfKeyContextOverLink
	                                      : xpdfKeyContextOffLink) |
            xpdfKeyContextScrLockOff;
  return context;
}

int XpdfViewer::getMouseButton(Qt::MouseButton qtBtn) {
  if (qtBtn & Qt::LeftButton) {
    return 1;
  }
  if (qtBtn & Qt::MiddleButton) {
    return 2;
  }
  if (qtBtn & Qt::RightButton) {
    return 3;
  } 
  return 0;
}

// Grab any keyboard events that filter down to the window, and feed
// them to the main key processing function.
void XpdfViewer::keyPressEvent(QKeyEvent *e) {
  keyPress(e);
}

void XpdfViewer::pageChange(int pg) {
  pageNumber->setText(QString().setNum(pg));
  updateZoomInfo();
  updateOutline(pg);
}

#if XPDFWIDGET_PRINTING
void XpdfViewer::printStatus(int nextPage, int firstPage, int lastPage) {
  if (!printStatusDialog) {
    printStatusDialog = new QProgressDialog("Printing...", "Cancel",
					    firstPage, lastPage + 1, this);
    printStatusDialog->setWindowModality(Qt::WindowModal);
    printStatusDialog->setMinimumDuration(0);
    printStatusDialog->setAutoClose(false);
    printStatusDialog->setAutoReset(false);
    connect(printStatusDialog, SIGNAL(canceled()), this, SLOT(cancelPrint()));
    printStatusDialog->move(
		 pos().x() + (width() - printStatusDialog->width()) / 2,
		 pos().y() + (height() - printStatusDialog->height()) / 2);
    printStatusDialog->show();
  }
  printStatusDialog->setValue(nextPage);
  if (nextPage > lastPage) {
    printStatusDialog->cancel();
    delete printStatusDialog;
    printStatusDialog = NULL;
  }
  QApplication::processEvents();
}

void XpdfViewer::cancelPrint() {
  currentTab->pdf->cancelPrint();
}
#endif

//------------------------------------------------------------------------
// menu/toolbar actions
//------------------------------------------------------------------------

void XpdfViewer::openMenuAction() {
  execCmd("open", NULL);
}

void XpdfViewer::openInNewWinMenuAction() {
  execCmd("openInNewWin", NULL);
}

void XpdfViewer::reloadMenuAction() {
  execCmd("reload", NULL);
}

void XpdfViewer::saveAsMenuAction() {
  execCmd("saveAs", NULL);
}

void XpdfViewer::saveImageMenuAction() {
  execCmd("saveImage", NULL);
}

#if XPDFWIDGET_PRINTING
void XpdfViewer::printMenuAction() {
  execCmd("print", NULL);
}
#endif

void XpdfViewer::quitMenuAction() {
  execCmd("quit", NULL);
}

void XpdfViewer::copyMenuAction() {
  execCmd("copy", NULL);
}

void XpdfViewer::singlePageModeMenuAction() {
  execCmd("singlePageMode", NULL);
}

void XpdfViewer::continuousModeMenuAction() {
  execCmd("continuousMode", NULL);
}

void XpdfViewer::sideBySideSingleModeMenuAction() {
  execCmd("sideBySideSingleMode", NULL);
}

void XpdfViewer::sideBySideContinuousModeMenuAction() {
  execCmd("sideBySideContinuousMode", NULL);
}

void XpdfViewer::horizontalContinuousModeMenuAction() {
  execCmd("horizontalContinuousMode", NULL);
}

void XpdfViewer::fullScreenMenuAction(bool checked) {
  execCmd(checked ? "fullScreenMode" : "windowMode", NULL); 
}

void XpdfViewer::rotateClockwiseMenuAction() {
  execCmd("rotateCW", NULL);
}

void XpdfViewer::rotateCounterclockwiseMenuAction() {
  execCmd("rotateCCW", NULL);
}

void XpdfViewer::zoomToSelectionMenuAction() {
  execCmd("zoomToSelection", NULL);
}



void XpdfViewer::newTabMenuAction() {
  execCmd("newTab", NULL);
}

void XpdfViewer::newWindowMenuAction() {
  execCmd("newWindow", NULL);
}

void XpdfViewer::closeTabMenuAction() {
  execCmd("closeTabOrQuit", NULL);
}

void XpdfViewer::closeWindowMenuAction() {
  execCmd("closeWindowOrQuit", NULL);
}

void XpdfViewer::openErrorWindowMenuAction() {
  execCmd("openErrorWindow", NULL);
}

void XpdfViewer::helpMenuAction() {
  execCmd("help", NULL);
}

void XpdfViewer::aboutMenuAction() {
  execCmd("about", NULL);
}

void XpdfViewer::popupMenuAction(int idx) {
  PopupMenuCmd *cmd;
  int i;

  cmd = globalParams->getPopupMenuCmd(idx);
  for (i = 0; i < cmd->cmds->getLength(); ++i) {
    execCmd(((GString *)cmd->cmds->get(i))->getCString(), NULL);
  }
}

void XpdfViewer::pageNumberChanged() {
  int pg;
  GString *cmd;

  pg = pageNumber->text().toInt();
  cmd = GString::format("gotoPage({0:d})", pg);
  execCmd(cmd->getCString(), NULL);
  delete cmd;
  // after moving to a new page, focus goes to the XpdfWidget
  currentTab->pdf->setFocus(Qt::OtherFocusReason);
}

void XpdfViewer::backButtonPressed() {
  execCmd("goBackward", NULL);
}

void XpdfViewer::forwardButtonPressed() {
  execCmd("goForward", NULL);
}

void XpdfViewer::zoomOutButtonPressed() {
  execCmd("zoomOut", NULL);
}

void XpdfViewer::zoomInButtonPressed() {
  execCmd("zoomIn", NULL);
}

void XpdfViewer::zoomIndexChanged(const QString &zoomText) {
  QString z;
  GString *cmd;

  if (zoomText.endsWith("%")) {
    z = zoomText.left(zoomText.size() - 1);
  } else {
    z = zoomText;
  }
  cmd = GString::format("zoomPercent({0:s})", z.toLatin1().constData());
  execCmd(cmd->getCString(), NULL);
  delete cmd;
}

void XpdfViewer::zoomEditingFinished() {
  QString z;
  GString *cmd;

  z = zoomComboBox->currentText();
  if (z.endsWith("%")) {
    z = z.left(z.size() - 1);
  }
  cmd = GString::format("zoomPercent({0:s})", z.toLatin1().constData());
  execCmd(cmd->getCString(), NULL);
  delete cmd;
}

void XpdfViewer::fitWidthButtonPressed() {
  execCmd("zoomFitWidth", NULL);
}

void XpdfViewer::fitPageButtonPressed() {
  execCmd("zoomFitPage", NULL);
}

void XpdfViewer::selectModeButtonPressed() {
  execCmd("toggleSelectMode", NULL);
}

void XpdfViewer::statusIndicatorPressed() {
  execCmd("openErrorWindow", NULL);
}

void XpdfViewer::findTextChanged() {
  execCmd("findFirst", NULL);
}

void XpdfViewer::findNextButtonPressed() {
  execCmd("findNext", NULL);
}

void XpdfViewer::findPrevButtonPressed() {
  execCmd("findPrevious", NULL);
}

void XpdfViewer::newTabButtonPressed() {
  execCmd("newTab", NULL);
}

void XpdfViewer::switchTab(QListWidgetItem *current,
			   QListWidgetItem *previous) {
  XpdfTabInfo *tab;
  int i;

  for (i = 0; i < tabInfo->getLength(); ++i) {
    tab = (XpdfTabInfo *)tabInfo->get(i);
    if (tab->listItem == current) {
      gotoTab(i);
      return;
    }
  }
}

void XpdfViewer::infoComboBoxChanged(int idx) {
  updateInfoPane();
}

void XpdfViewer::outlineItemClicked(const QModelIndex& idx) {
  currentTab->pdf->gotoOutlineTarget((XpdfOutlineHandle)idx.internalPointer());
  updateModeInfo();
}

void XpdfViewer::layerItemClicked(const QModelIndex& idx) {
  if (idx.data(Qt::CheckStateRole) == Qt::Checked) {
    currentTab->layerTree->model()->setData(idx, Qt::Unchecked,
					    Qt::CheckStateRole);
  } else {
    currentTab->layerTree->model()->setData(idx, Qt::Checked,
					    Qt::CheckStateRole);
  }
}

void XpdfViewer::attachmentSaveClicked(int idx) {
  QString fileName;

  fileName = QFileDialog::getSaveFileName(this, "Save Attachment");
  if (fileName.isEmpty()) {
    return;
  }
  currentTab->pdf->saveEmbeddedFile(idx, fileName);
}

//------------------------------------------------------------------------
// GUI setup
//------------------------------------------------------------------------

void XpdfViewer::createWindow() {
  errorWindow = new XpdfErrorWindow(this, app->getErrorEventType());

  setWindowIcon(QIcon(":/xpdf-icon"));

  createMainMenu();

  createXpdfPopupMenu();

  createToolBar();
  addToolBar(toolBar);
  setUnifiedTitleAndToolBarOnMac(true);

  sidebarSplitter = new QSplitter(Qt::Horizontal);
  setCentralWidget(sidebarSplitter);

  QSplitter *vSplitter = new QSplitter(Qt::Vertical);
  sidebarSplitter->addWidget(vSplitter);

  QWidget *tabPane = createTabPane();
  vSplitter->addWidget(tabPane);

  QWidget *infoPane = createInfoPane();
  vSplitter->addWidget(infoPane);

  QList<int> vSplitterSizes;
  vSplitterSizes.append(200);
  vSplitterSizes.append(600);
  vSplitter->setSizes(vSplitterSizes);

  viewerStack = new QStackedWidget();
  sidebarSplitter->addWidget(viewerStack);

  QList<int> sidebarSplitterSizes = sidebarSplitter->sizes();
  if (!globalParams->getInitialSidebarState()) {
    sidebarSplitterSizes[0] = 0;
    sidebarSplitterSizes[1] = 1;
    sidebarSplitter->setSizes(sidebarSplitterSizes);
  }
  // note: this is just an arbitrary initial value for sidebarWidth;
  // it will be updated by open/close/toggleSidebar
  sidebarWidth = 200;

  linkTargetBar = new QLabel(this);
  linkTargetBar->setStyleSheet("padding:2px; background:#00ffff;");
  linkTargetBar->setAttribute(Qt::WA_TransparentForMouseEvents, true);

  findErrorTimer = new QTimer(this);
  findErrorTimer->setSingleShot(true);
  connect(findErrorTimer, SIGNAL(timeout()), this, SLOT(clearFindError()));

  aboutDialog = NULL;
#if XPDFWIDGET_PRINTING
  printStatusDialog = NULL;
#endif

  tabInfo = new GList();
  addTab();
  updateModeInfo();
  updateDocInfo();
}

void XpdfViewer::createToolBar() {
  QString zoomVal;
  int i;

  toolBar = new QToolBar();
  toolBar->setFloatable(false);
  toolBar->setMovable(false);

  //--- page number and page count
  pageNumber = new QLineEdit();
  toolBarFontSize = pageNumber->sizeHint().height();
  toolBar->setIconSize(QSize(toolBarFontSize - 2, toolBarFontSize - 2));
  //~ not sure why the magic "+6" is needed
  pageNumber->setFixedWidth(pageNumber->fontMetrics().width("00000") + 6);
  pageNumber->setToolTip("current page number");
  toolBar->addWidget(pageNumber);
  addToolBarSpacing(2);
  toolBar->addWidget(new QLabel("/"));
  addToolBarSpacing(2);
  connect(pageNumber, SIGNAL(returnPressed()), this, SLOT(pageNumberChanged()));
  pageCount = new QLabel("");
  pageCount->setMinimumWidth(pageNumber->fontMetrics().width("0000"));
  pageCount->setToolTip("page count");
  toolBar->addWidget(pageCount);
  addToolBarSeparator();

  //--- back / forward buttons
  addToolBarButton(QIcon(":/back-button"),
		   SLOT(backButtonPressed()), "back to previous view");
  addToolBarButton(QIcon(":/forward-button"),
		   SLOT(forwardButtonPressed()), "forward to next view");
  addToolBarSeparator();

  //--- zoom controls
  addToolBarButton(QIcon(":/zoomOut-button"),
		   SLOT(zoomOutButtonPressed()), "zoom out");
  addToolBarButton(QIcon(":/zoomIn-button"),
		   SLOT(zoomInButtonPressed()), "zoom in");
  addToolBarSpacing(4);
  zoomComboBox = new QComboBox();
  zoomComboBox->setToolTip("change zoom level");
  for (i = 0; i < nZoomComboBoxVals; ++i) {
    zoomVal.setNum(zoomComboBoxVals[i]);
    zoomVal.append('%');
    zoomComboBox->addItem(zoomVal);
  }
  zoomComboBox->setEditable(true);
  zoomComboBox->setInsertPolicy(QComboBox::NoInsert);
  zoomComboBox->setValidator(new ZoomValidator(this));
  connect(zoomComboBox, SIGNAL(currentIndexChanged(const QString&)),
	  this, SLOT(zoomIndexChanged(const QString&)));
  // this could use the editingFinished signal, but that's emitted
  // every time the popup is opened and closed, which causes the zoom
  // level to be reset
  connect(zoomComboBox->lineEdit(), SIGNAL(returnPressed()),
	  this, SLOT(zoomEditingFinished()));
  toolBar->addWidget(zoomComboBox);
  addToolBarSpacing(4);
  fitWidthBtn = addToolBarButton(QIcon(":/fitWidth-button"),
				 SLOT(fitWidthButtonPressed()),
				 "fit page width to window");
  fitPageBtn = addToolBarButton(QIcon(":/fitPage-button"),
				SLOT(fitPageButtonPressed()),
				"fit page to window");
  addToolBarSeparator();

  //--- selection mode toggle
  selectModeBtn = addToolBarButton(QIcon(":/selectModeLinear-button"),
				   SLOT(selectModeButtonPressed()),
				   "toggle selection mode");
  addToolBarSeparator();

  //--- status indicator
  QToolButton *indicatorBtn =
      addToolBarButton(QIcon(":/indicator-icon0"),
		       SLOT(statusIndicatorPressed()),
		       "click to open error window");
  indicatorIcons.append(QIcon(":/indicator-icon0"));
  indicatorIcons.append(QIcon(":/indicator-icon1"));
  indicatorIcons.append(QIcon(":/indicator-icon2"));
  indicatorIcons.append(QIcon(":/indicator-icon3"));
  indicatorIcons.append(QIcon(":/indicator-icon4"));
  indicatorIcons.append(QIcon(":/indicator-icon5"));
  indicatorIcons.append(QIcon(":/indicator-icon6"));
  indicatorIcons.append(QIcon(":/indicator-icon7"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err0"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err1"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err2"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err3"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err4"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err5"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err6"));
  indicatorErrIcons.append(QIcon(":/indicator-icon-err7"));
  indicatorAnimation = new PropertyListAnimation(indicatorBtn, "icon",
						 indicatorIcons);
  indicatorAnimation->setDuration(1000);
  indicatorAnimation->setLoopCount(-1);
  indicatorAnimation->setStartValue(indicatorIcons[0]);
  indicatorAnimation->setEndValue(indicatorIcons[7]);
  indicatorAnimation->start();
  indicatorAnimation->pause();

  //--- find controls
  addToolBarStretch();
  findEdit = new QLineEdit();
  findEdit->setPlaceholderText("find");
  findEdit->setFixedWidth(20 * findEdit->fontMetrics().width("0"));
  toolBar->addWidget(findEdit);
  connect(findEdit, SIGNAL(returnPressed()), this, SLOT(findTextChanged()));
  connect(findEdit, SIGNAL(cursorPositionChanged(int, int)),
	  this, SLOT(clearFindError()));
  connect(findEdit, SIGNAL(selectionChanged()), this, SLOT(clearFindError()));
  connect(findEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(clearFindError()));
  addToolBarButton(QIcon(":/findNext-button"),
		   SLOT(findNextButtonPressed()), "find next occurrence");
  addToolBarButton(QIcon(":/findPrevious-button"),
		   SLOT(findPrevButtonPressed()), "find previous occurrence");
  QMenu *findSettingsMenu = new QMenu(this);
  findCaseSensitiveAction = findSettingsMenu->addAction("case sensitive");
  findCaseSensitiveAction->setCheckable(true);
  findWholeWordsAction = findSettingsMenu->addAction("whole words");
  findWholeWordsAction->setCheckable(true);
  addToolBarMenuButton(QIcon(":/findSettings-button"),
		       "change find settings", findSettingsMenu);
}

QToolButton *XpdfViewer::addToolBarButton(const QIcon &icon,
					  const char *slot, const char *tip) {
  QAction *action = new QAction(icon, "", this);
  action->setToolTip(tip);
  QToolButton *button = new QToolButton();
  button->setDefaultAction(action);
  button->setAutoRaise(true);
  toolBar->addWidget(button);
  connect(action, SIGNAL(triggered()), this, slot);
  return button;
}

XpdfMenuButton *XpdfViewer::addToolBarMenuButton(const QIcon &icon,
						 const char *tip,
						 QMenu *menu) {
  QAction *action = new QAction(icon, "", this);
  action->setToolTip(tip);
  XpdfMenuButton *button = new XpdfMenuButton(menu);
  button->setDefaultAction(action);
  button->setAutoRaise(true);
  button->setToolTip(tip);
  toolBar->addWidget(button);
  return button;
}

void XpdfViewer::addToolBarSeparator() {
  addToolBarSpacing(8);
  toolBar->addSeparator();
  addToolBarSpacing(8);
}

void XpdfViewer::addToolBarSpacing(int w) {
  QWidget *space = new QWidget();
  space->setFixedWidth((toolBarFontSize * w) / 20);
  toolBar->addWidget(space);
}

void XpdfViewer::addToolBarStretch() {
  QWidget *stretch = new QWidget();
  stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  toolBar->addWidget(stretch);
}

void XpdfViewer::createMainMenu() {
  mainMenu = menuBar();

  QMenu *fileSubmenu = mainMenu->addMenu("&File");
  fileSubmenu->addAction("&Open...", this, SLOT(openMenuAction()));
  fileSubmenu->addAction("Open in new window...",
			 this, SLOT(openInNewWinMenuAction()));
  fileSubmenu->addAction("Reload", this, SLOT(reloadMenuAction()));
  fileSubmenu->addAction("&Save as...", this, SLOT(saveAsMenuAction()));
  fileSubmenu->addSeparator();
  fileSubmenu->addAction("Save image...", this, SLOT(saveImageMenuAction()));
#if XPDFWIDGET_PRINTING
  fileSubmenu->addSeparator();
  fileSubmenu->addAction("&Print...", this, SLOT(printMenuAction()));
#endif
  fileSubmenu->addSeparator();
#ifdef _WIN32
  fileSubmenu->addAction("E&xit", this, SLOT(quitMenuAction()));
#else
  fileSubmenu->addAction("&Quit", this, SLOT(quitMenuAction()));
#endif

  QMenu *editSubmenu = mainMenu->addMenu("&Edit");
  editSubmenu->addAction("Copy", this, SLOT(copyMenuAction()));

  QMenu *viewSubmenu = mainMenu->addMenu("&View");
  displayModeSubmenu = new QMenu(this);
  QActionGroup *displayModeGroup = new QActionGroup(this);
  QAction *action;
  action = displayModeSubmenu->addAction(
	       "Single page",
	       this, SLOT(singlePageModeMenuAction()));
  action->setCheckable(true);
  displayModeGroup->addAction(action);
  action = displayModeSubmenu->addAction(
	       "Continuous",
	       this, SLOT(continuousModeMenuAction()));
  action->setCheckable(true);
  displayModeGroup->addAction(action);
  action = displayModeSubmenu->addAction(
	       "Side-by-side single",
	       this, SLOT(sideBySideSingleModeMenuAction()));
  action->setCheckable(true);
  displayModeGroup->addAction(action);
  action = displayModeSubmenu->addAction(
	       "Side-by-side continuous",
	       this, SLOT(sideBySideContinuousModeMenuAction()));
  action->setCheckable(true);
  displayModeGroup->addAction(action);
  action = displayModeSubmenu->addAction(
	       "Horizontal continuous",
	       this, SLOT(horizontalContinuousModeMenuAction()));
  action->setCheckable(true);
  displayModeGroup->addAction(action);
  viewSubmenu->addAction("Display mode")->setMenu(displayModeSubmenu);
  fullScreenMenuItem = viewSubmenu->addAction("Full screen", this,
					      SLOT(fullScreenMenuAction(bool)));
  fullScreenMenuItem->setCheckable(true);
  viewSubmenu->addSeparator();
  viewSubmenu->addAction("Rotate clockwise",
			 this, SLOT(rotateClockwiseMenuAction()));
  viewSubmenu->addAction("Rotate counterclockwise",
			 this, SLOT(rotateCounterclockwiseMenuAction()));
  viewSubmenu->addSeparator();
  viewSubmenu->addAction("Zoom to selection",
			 this, SLOT(zoomToSelectionMenuAction()));


  QMenu *windowSubmenu = mainMenu->addMenu("&Window");
  windowSubmenu->addAction("New tab", this, SLOT(newTabMenuAction()));
  windowSubmenu->addAction("New window", this, SLOT(newWindowMenuAction()));
  windowSubmenu->addSeparator();
  windowSubmenu->addAction("Close tab", this, SLOT(closeTabMenuAction()));
  windowSubmenu->addAction("Close window", this, SLOT(closeWindowMenuAction()));
  windowSubmenu->addSeparator();
  windowSubmenu->addAction("Open error window...",
			   this, SLOT(openErrorWindowMenuAction()));

  QMenu *helpSubmenu = mainMenu->addMenu("&Help");
  helpSubmenu->addAction("Help...", this, SLOT(helpMenuAction()));
  helpSubmenu->addAction("About XpdfReader...", this, SLOT(aboutMenuAction()));
}

// This can't be named createPopupMenu because there's a QMainWindow
// function of that name.
void XpdfViewer::createXpdfPopupMenu() {
  PopupMenuCmd *cmd;
  QAction *action;
  int n, i;

  n = globalParams->getNumPopupMenuCmds();
  if (n == 0) {
    popupMenu = NULL;
    return;
  }

  popupMenu = new QMenu(this);
  popupMenuSignalMapper = new QSignalMapper(this);
  connect(popupMenuSignalMapper, SIGNAL(mapped(int)),
	  this, SLOT(popupMenuAction(int)));

  for (i = 0; i < n; ++i) {
    cmd = globalParams->getPopupMenuCmd(i);
    action = popupMenu->addAction(cmd->label->getCString(),
				  popupMenuSignalMapper, SLOT(map()));
    popupMenuSignalMapper->setMapping(action, i);
  }
}

QWidget *XpdfViewer::createTabPane() {
  QWidget *tabPane = new QWidget();

  QVBoxLayout *tabPaneLayout = new QVBoxLayout();
  tabPaneLayout->setContentsMargins(0, 0, 0, 0);
  tabPaneLayout->setSpacing(0);
  tabPane->setLayout(tabPaneLayout);

  tabList = new QListWidget();
  tabList->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(tabList, SIGNAL(currentItemChanged(QListWidgetItem*,
					     QListWidgetItem*)),
	  this, SLOT(switchTab(QListWidgetItem*, QListWidgetItem*)));
  tabPaneLayout->addWidget(tabList);

  QPushButton *newTabBtn = new QPushButton("+ tab");
  connect(newTabBtn, SIGNAL(clicked()), this, SLOT(newTabButtonPressed()));
  tabPaneLayout->addWidget(newTabBtn);

  return tabPane;
}

QWidget *XpdfViewer::createInfoPane() {
  QWidget *infoPane = new QWidget();

  QVBoxLayout *infoLayout = new QVBoxLayout();
  infoLayout->setContentsMargins(0, 0, 0, 0);
  infoLayout->setSpacing(0);
  infoPane->setLayout(infoLayout);

  // NB: order here must match order in updateInfoPane().
  infoComboBox = new QComboBox();
  infoComboBox->setEditable(false);
  infoComboBox->addItem("outline");
  infoComboBox->addItem("layers");
  infoComboBox->addItem("attachments");
  infoLayout->addWidget(infoComboBox);
  connect(infoComboBox, SIGNAL(activated(int)),
	  this, SLOT(infoComboBoxChanged(int)));

  infoStack = new QStackedLayout();
  infoLayout->addLayout(infoStack);

  return infoPane;
}

void XpdfViewer::updateInfoPane() {
  // NB: order here must match order in createInfoPane().
  switch (infoComboBox->currentIndex()) {
  case 0:
    infoStack->setCurrentWidget(currentTab->outlineTree);
    break;
  case 1:
    infoStack->setCurrentWidget(currentTab->layerTree);
    break;
  case 2:
    infoStack->setCurrentWidget(currentTab->attachmentList);
    break;
  }
}

void XpdfViewer::destroyWindow() {
  int i;

  delete errorWindow;

  // QTreeView doesn't take ownership of the model, so we need to
  // explicitly delete those
  for (i = 0; i < tabInfo->getLength(); ++i) {
    delete ((XpdfTabInfo *)tabInfo->get(i))->outlineTree->model();
    delete ((XpdfTabInfo *)tabInfo->get(i))->layerTree->model();
  }

  deleteGList(tabInfo, XpdfTabInfo);

  delete indicatorAnimation;
}

void XpdfViewer::enterFullScreenMode() {
  mainMenu->hide();
  toolBar->hide();
  sidebarSplitter->widget(0)->hide();

  fullScreenPreviousDisplayMode = currentTab->pdf->getDisplayMode();
  currentTab->pdf->setDisplayMode(XpdfWidget::pdfDisplaySingle);
  updateModeInfo();

  fullScreenPreviousZoom = currentTab->pdf->getZoom();
  currentTab->pdf->setZoom(XpdfWidget::zoomToPage);
  updateZoomInfo();

  currentTab->pdf->setMatteColor(app->getFullScreenMatteColor());
  currentTab->pdf->setFrameStyle(QFrame::NoFrame);

  showFullScreen();

  fullScreenMenuItem->setChecked(true);
}

void XpdfViewer::exitFullScreenMode() {
  mainMenu->show();
  toolBar->show();
  sidebarSplitter->widget(0)->show();

  currentTab->pdf->setDisplayMode(fullScreenPreviousDisplayMode);
  updateModeInfo();

  currentTab->pdf->setZoom(fullScreenPreviousZoom);
  updateZoomInfo();

  currentTab->pdf->setMatteColor(app->getMatteColor());
  currentTab->pdf->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);

  showNormal();

  fullScreenMenuItem->setChecked(false);
}

void XpdfViewer::addTab() {
  QListWidgetItem *listItem;
  XpdfWidget *pdf;
  QTreeView *outlineTree, *layerTree;
  QTableWidget *attachmentList;

  pdf = new XpdfWidget(NULL, app->getPaperColor(), app->getMatteColor(),
		       app->getReverseVideo());
  pdf->enableHyperlinks(false);
  pdf->setKeyPassthrough(true);
  pdf->setMousePassthrough(true);
  pdf->setLinearSelectMode();
  connect(pdf, SIGNAL(resized()), this, SLOT(pdfResized()));
  connect(pdf, SIGNAL(paintDone(bool)), this, SLOT(pdfPaintDone(bool)));
  connect(pdf, SIGNAL(preLoad()), this, SLOT(preLoad()));
  connect(pdf, SIGNAL(postLoad()), this, SLOT(postLoad()));
  connect(pdf, SIGNAL(keyPress(QKeyEvent*)),
	  this, SLOT(keyPress(QKeyEvent*)));
  connect(pdf, SIGNAL(mousePress(QMouseEvent*)),
	  this, SLOT(mousePress(QMouseEvent*)));
  connect(pdf, SIGNAL(mouseRelease(QMouseEvent*)),
	  this, SLOT(mouseRelease(QMouseEvent*)));
  connect(pdf, SIGNAL(mouseWheel(QWheelEvent*)),
	  this, SLOT(mouseWheel(QWheelEvent*)));
  connect(pdf, SIGNAL(mouseMove(QMouseEvent*)),
	  this, SLOT(mouseMove(QMouseEvent*)));
  connect(pdf, SIGNAL(midPageChange(int)), this, SLOT(pageChange(int)));
#if XPDFWIDGET_PRINTING
  connect(pdf, SIGNAL(printStatus(int, int, int)),
	  this, SLOT(printStatus(int, int, int)));
#endif
  viewerStack->addWidget(pdf);
  viewerStack->setCurrentWidget(pdf);
  // after adding a tab, focus goes to the XpdfWidget
  pdf->setFocus(Qt::OtherFocusReason);
 
  //--- create tab pane item
  listItem = new QListWidgetItem();
  tabList->addItem(listItem);
  tabList->setCurrentItem(listItem);

  //--- create outline view
  outlineTree = new QTreeView();
  outlineTree->setModel(new OutlineModel(pdf));
  outlineTree->setHeaderHidden(true);
  outlineTree->setUniformRowHeights(true);
  outlineTree->setSelectionMode(QAbstractItemView::SingleSelection);
  connect(outlineTree, SIGNAL(clicked(const QModelIndex&)),
	  this, SLOT(outlineItemClicked(const QModelIndex&)));
  infoStack->addWidget(outlineTree);

  //--- create layer view
  layerTree = new QTreeView();
  layerTree->setModel(new LayerModel(pdf));
  layerTree->setHeaderHidden(true);
  layerTree->setUniformRowHeights(true);
  layerTree->setSelectionMode(QAbstractItemView::NoSelection);
  connect(layerTree, SIGNAL(clicked(const QModelIndex&)),
	  this, SLOT(layerItemClicked(const QModelIndex&)));
  infoStack->addWidget(layerTree);

  //--- create attachment list
  attachmentList = new QTableWidget(4, 2);
  attachmentList->horizontalHeader()->hide();
  attachmentList->verticalHeader()->hide();
  attachmentList->setShowGrid(false);
  attachmentList->setWordWrap(false);
  attachmentList->setSelectionMode(QAbstractItemView::NoSelection);
  infoStack->addWidget(attachmentList);

  currentTab = new XpdfTabInfo(listItem, pdf, outlineTree, layerTree,
			       attachmentList);
  tabInfo->append(currentTab);

  updateInfoPane();
}

void XpdfViewer::closeTab(XpdfTabInfo *tab) {
  int i;

  for (i = 0; i < tabInfo->getLength(); ++i) {
    if ((XpdfTabInfo *)tabInfo->get(i) == tab) {
      break;
    }
  }
  if (i == tabInfo->getLength()) {
    // this shouldn't happen
    return;
  }
  tabInfo->del(i);

  for (i = 0; i < tabList->count(); ++i) {
    if (tabList->item(i) == tab->listItem) {
      delete tabList->takeItem(i);
      break;
    }
  }

  infoStack->removeWidget(tab->outlineTree);
  delete tab->outlineTree->model();
  delete tab->outlineTree;
  infoStack->removeWidget(tab->layerTree);
  delete tab->layerTree->model();
  delete tab->layerTree;
  infoStack->removeWidget(tab->attachmentList);

  viewerStack->removeWidget(tab->pdf);
  tab->pdf->closeFile();
  delete tab->pdf;

  delete tab;

  if (tabInfo->getLength() > 0) {
    updateModeInfo();
    updateDocInfo();
  }
}

void XpdfViewer::gotoTab(int idx) {
  XpdfTabInfo *tab;

  tab = (XpdfTabInfo *)tabInfo->get(idx);
  currentTab = tab;
  viewerStack->setCurrentWidget(currentTab->pdf);
  // after switching tabs, focus goes to the XpdfWidget
  currentTab->pdf->setFocus(Qt::OtherFocusReason);
  updateInfoPane();
  updateModeInfo();
  updateDocInfo();
}

// Update the display mode checkboxes, based on the current XpdfWidget
// settings.
void XpdfViewer::updateModeInfo() {
  switch (currentTab->pdf->getDisplayMode()) {
  case XpdfWidget::pdfDisplaySingle:
    displayModeSubmenu->actions()[0]->setChecked(true);
    break;
  case XpdfWidget::pdfDisplayContinuous:
    displayModeSubmenu->actions()[1]->setChecked(true);
    break;
  case XpdfWidget::pdfDisplaySideBySideSingle:
    displayModeSubmenu->actions()[2]->setChecked(true);
    break;
  case XpdfWidget::pdfDisplaySideBySideContinuous:
    displayModeSubmenu->actions()[3]->setChecked(true);
    break;
  case XpdfWidget::pdfDisplayHorizontalContinuous:
    displayModeSubmenu->actions()[4]->setChecked(true);
    break;
  }
}

// Update the displayed zoom percentage, based on the current
// XpdfWidget settings.
void XpdfViewer::updateZoomInfo() {
  int pg;
  double z;
  QString zoomStr;

  if (currentTab->pdf->hasOpenDocument()) {
    pg = currentTab->pdf->getMidPage();
  } else {
    pg = 1;
  }
  z = (int)floor(currentTab->pdf->getZoomPercent(pg) + 0.5);
  zoomStr.setNum(z);
  zoomStr.append('%');
  zoomComboBox->setEditText(zoomStr);

  z = currentTab->pdf->getZoom();
  if (z == XpdfWidget::zoomToWidth) {
    fitWidthBtn->setIcon(QIcon(":/fitWidthOn-button"));
    fitPageBtn->setIcon(QIcon(":/fitPage-button"));
  } else if (z == XpdfWidget::zoomToPage) {
    fitWidthBtn->setIcon(QIcon(":/fitWidth-button"));
    fitPageBtn->setIcon(QIcon(":/fitPageOn-button"));
  } else {
    fitWidthBtn->setIcon(QIcon(":/fitWidth-button"));
    fitPageBtn->setIcon(QIcon(":/fitPage-button"));
  }
}

void XpdfViewer::updateSelectModeInfo() {
  if (currentTab->pdf->isBlockSelectMode()) {
    selectModeBtn->setIcon(QIcon(":/selectModeBlock-button"));
  } else {
    selectModeBtn->setIcon(QIcon(":/selectModeLinear-button"));
  }
}

// This is called when:
//   - when the GUI is initially created
//   - a document is opened or closed
//   - a tab switch happens
// It updates all visible info related to the document.
void XpdfViewer::updateDocInfo() {
  //--- window title
  QString windowTitle("XpdfReader");
  if (currentTab->pdf->hasOpenDocument()) {
    windowTitle += ": ";
    windowTitle += currentTab->pdf->getFileName();
  }
  setWindowTitle(windowTitle);

  //--- tab title
  QString tabTitle;
  if (currentTab->pdf->hasOpenDocument()) {
    tabTitle = currentTab->pdf->getFileName();
  } else {
    tabTitle = "empty";
  }
  currentTab->listItem->setText(tabTitle);
  currentTab->listItem->setToolTip(tabTitle);

  //--- page number
  pageNumber->setText(QString().setNum(currentTab->pdf->getMidPage()));

  //--- page count
  QString nPages;
  if (currentTab->pdf->hasOpenDocument()) {
    nPages.setNum(currentTab->pdf->getNumPages());
  }
  pageCount->setText(nPages);

  //--- zoom
  // NB: in fit-{page,width,height} modes zoom percent depends on page
  // size, so we need to update whenever a new doc is loaded
  updateZoomInfo();

  //--- selection mode
  updateSelectModeInfo();

  //--- hide the link target bar
  currentTab->pdf->unsetCursor();
  linkTargetBar->hide();
  linkTargetInfo = QString();
}

void XpdfViewer::updateOutline(int pg) {
  QModelIndex idx;

  idx = ((OutlineModel *)currentTab->outlineTree->model())
                             ->findPageIndex(pg, currentTab->outlineTree);
  if (idx.isValid()) {
    currentTab->outlineTree->setCurrentIndex(idx);
  }
}

void XpdfViewer::setOutlineOpenItems(const QModelIndex &idx) {
  OutlineModel *model;
  XpdfOutlineHandle item;
  QModelIndex child;
  int n, i;

  if (idx.isValid()) {
    item = (XpdfOutlineHandle)idx.internalPointer();
    if (!currentTab->pdf->getOutlineStartsOpen(item)) {
      return;
    }
    currentTab->outlineTree->expand(idx);
  }
  model = (OutlineModel *)currentTab->outlineTree->model();
  n = model->rowCount(idx);
  for (i = 0; i < n; ++i) {
    child = model->index(i, 0, idx);
    setOutlineOpenItems(child);
  }
}

void XpdfViewer::fillAttachmentList() {
  QButtonGroup *btnGroup;
  QPushButton *saveBtn;
  QTableWidgetItem *item;
  int n, i;

  n = currentTab->pdf->getNumEmbeddedFiles();
  currentTab->attachmentList->setRowCount(n);
  currentTab->attachmentList->setColumnCount(2);
  btnGroup = new QButtonGroup(currentTab->attachmentList);
  for (i = 0; i < n; ++i) {
    saveBtn = new QPushButton("save");
    saveBtn->setStyleSheet("padding-left:4px; padding-right:4px;");
    btnGroup->addButton(saveBtn, i);
    currentTab->attachmentList->setCellWidget(i, 0, saveBtn);
    item = new QTableWidgetItem(currentTab->pdf->getEmbeddedFileName(i));
    currentTab->attachmentList->setItem(i, 1, item);
  }
  connect(btnGroup, SIGNAL(buttonClicked(int)),
	  this, SLOT(attachmentSaveClicked(int)));
  currentTab->attachmentList->resizeRowsToContents();
  currentTab->attachmentList->resizeColumnsToContents();
}

void XpdfViewer::statusIndicatorStart() {
  if (indicatorAnimation->state() == QAbstractAnimation::Paused) {
    indicatorAnimation->resume();
  }
}

void XpdfViewer::statusIndicatorStop() {
  if (indicatorAnimation->state() == QAbstractAnimation::Running) {
    indicatorAnimation->pause();
    indicatorAnimation->setCurrentTime(0);
  }
}

void XpdfViewer::statusIndicatorOk() {
  if (indicatorAnimation->values() != indicatorIcons) {
    indicatorAnimation->setValues(indicatorIcons);
  }
}

void XpdfViewer::statusIndicatorError() {
  if (indicatorAnimation->values() != indicatorErrIcons) {
    indicatorAnimation->setValues(indicatorErrIcons);
  }
}

void XpdfViewer::showFindError() {
  findEdit->setStyleSheet("background: #ff8080;");
  findErrorTimer->start(1000);
}

void XpdfViewer::clearFindError() {
  findErrorTimer->stop();
  findEdit->setStyleSheet("");
}

void XpdfViewer::createAboutDialog() {
  aboutDialog = new QDialog(this);
  aboutDialog->setWindowTitle("About XpdfReader");
 
  QVBoxLayout *vbox = new QVBoxLayout();
  aboutDialog->setLayout(vbox);

  QTextBrowser *text = new QTextBrowser();
  text->setOpenExternalLinks(true);
  text->setHtml(aboutHTML);
  text->setReadOnly(true);
  text->setMinimumSize(QSize(500, 300));
  vbox->addWidget(text);

  QHBoxLayout *btnBox = new QHBoxLayout();
  vbox->addLayout(btnBox);

  QPushButton *closeBtn = new QPushButton("Close");
  closeBtn->setDefault(true);
  btnBox->addStretch(1);
  btnBox->addWidget(closeBtn);
  btnBox->addStretch(1);
  connect(closeBtn, SIGNAL(clicked()), aboutDialog, SLOT(accept()));
}

#define nSaveImageFormats 3
static struct {
  const char *comboBoxText;
  const char *fileFilter;
  const char *qImageFormat;
} saveImageFormats[nSaveImageFormats] = {
  { "JPEG", "JPEG files (*.jpg)",  "JPEG" },
  { "PNG",  "PNG files (*.png)",   "PNG" },
  { "TIFF", "TIFF files (*.tiff)", "TIFF" }
};

void XpdfViewer::execSaveImageDialog() {
  int i;

  QDialog *dialog = new QDialog();
  dialog->setWindowTitle("XpdfReader: Save Image");

  QVBoxLayout *vbox = new QVBoxLayout();
  dialog->setLayout(vbox);

  QGridLayout *grid = new QGridLayout();
  vbox->addLayout(grid);

  grid->addWidget(new QLabel("Region:"), 0, 0);

  QHBoxLayout *regionBox = new QHBoxLayout();
  grid->addLayout(regionBox, 0, 1);

  QButtonGroup *regionBtnGroup = new QButtonGroup(dialog);

  QRadioButton *pageBtn = new QRadioButton("Page:");
  regionBtnGroup->addButton(pageBtn);
  regionBox->addWidget(pageBtn);
  pageBtn->setChecked(true);

  QLineEdit *pageEdit = new QLineEdit();
  regionBox->addWidget(pageEdit);
  pageEdit->setFixedWidth(8 * pageEdit->fontMetrics().width("0"));
  int pg = currentTab->pdf->getMidPage();
  pageEdit->setText(QString().setNum(pg));
  connect(pageEdit, SIGNAL(textChanged(const QString&)),
	  pageBtn, SLOT(click()));
  connect(pageEdit, SIGNAL(cursorPositionChanged(int, int)),
	  pageBtn, SLOT(click()));
  connect(pageEdit, SIGNAL(selectionChanged()), pageBtn, SLOT(click()));

  regionBox->addSpacing(20);

  QRadioButton *selectionBtn = new QRadioButton("Current selection");
  regionBtnGroup->addButton(selectionBtn);
  regionBox->addWidget(selectionBtn);
  selectionBtn->setEnabled(currentTab->pdf->hasSelection());

  grid->addWidget(new QLabel("Resolution:"), 2, 0);

  QHBoxLayout *resolutionBox = new QHBoxLayout();
  grid->addLayout(resolutionBox, 2, 1);

  QLineEdit *resolutionEdit = new QLineEdit();
  resolutionBox->addWidget(resolutionEdit);
  resolutionEdit->setFixedWidth(8 * pageEdit->fontMetrics().width("0"));
  int r = (int)floor(currentTab->pdf->getZoomPercent(pg) * 0.72 + 0.5);
  resolutionEdit->setText(QString().setNum(r));

  resolutionBox->addWidget(new QLabel("dpi"));

  grid->addWidget(new QLabel("Format:"), 3, 0);

  QHBoxLayout *formatBox = new QHBoxLayout();
  grid->addLayout(formatBox, 3, 1);

  QComboBox *formatCombo = new QComboBox();
  formatBox->addWidget(formatCombo);
  formatCombo->setEditable(false);
  for (i = 0; i < nSaveImageFormats; ++i) {
    formatCombo->addItem(saveImageFormats[i].comboBoxText);
  }
  formatCombo->setCurrentIndex(0);

  formatBox->addStretch();

  QHBoxLayout *btnBox = new QHBoxLayout();
  vbox->addLayout(btnBox);

  btnBox->addStretch();

  QPushButton *cancelBtn = new QPushButton("Cancel");
  btnBox->addWidget(cancelBtn);
  connect(cancelBtn, SIGNAL(clicked()), dialog, SLOT(reject()));

  QPushButton *okBtn = new QPushButton("Ok");
  btnBox->addWidget(okBtn);
  okBtn->setDefault(true);
  connect(okBtn, SIGNAL(clicked()), dialog, SLOT(accept()));

  dialog->setModal(true);

  if (dialog->exec() == QDialog::Accepted) {
    double res = resolutionEdit->text().toDouble();
    bool wholePage = pageBtn->isChecked();
    int page;
    double x0, y0, x1, y1;
    if (wholePage) {
      page = pageBtn->text().toInt();
      if (page < 1 || page > currentTab->pdf->getNumPages()) {
	page = 1;
      }
      x0 = y0 = x1 = y1 = 0;
    } else {
      currentTab->pdf->getCurrentSelection(&page, &x0, &y0, &x1, &y1);
    }
    int fmt = formatCombo->currentIndex();
    QString fileName =
        QFileDialog::getSaveFileName(this, "Save Image",
				     QString(),
				     saveImageFormats[fmt].fileFilter);
    if (!fileName.isEmpty()) {
      QImage img;
      if (wholePage) {
	img = currentTab->pdf->convertPageToImage(page, res);
      } else {
	img = currentTab->pdf->convertRegionToImage(page, x0, y0, x1, y1, res);
      }
      img.save(fileName, saveImageFormats[fmt].qImageFormat);
    }
  }

  delete dialog;
}
