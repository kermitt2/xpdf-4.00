//========================================================================
//
// XpdfViewer.h
//
// Copyright 2015 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFVIEWER_H
#define XPDFVIEWER_H

#include <aconf.h>

#include <QDialog>
#include <QIcon>
#include <QMainWindow>
#include <QToolButton>
#include "gtypes.h"
#include "Error.h"
#include "XpdfWidget.h"

class GString;
class GList;
class PropertyListAnimation;
class QComboBox;
class QDialog;
class QHBoxLayout;
class QInputEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QModelIndex;
class QProgressDialog;
class QSignalMapper;
class QSplitter;
class QStackedLayout;
class QStackedWidget;
class QTimer;
class QToolBar;
class QToolButton;
class XpdfApp;
class XpdfMenuButton;
class XpdfTabInfo;
class XpdfViewer;

//------------------------------------------------------------------------

struct XpdfViewerCmd {
  const char *name;
  int nArgs;
  GBool requiresDoc;
  GBool requiresEvent;
  void (XpdfViewer::*func)(GString *args[], int nArgs, QInputEvent *event);
};

//------------------------------------------------------------------------
// XpdfMenuButton
//------------------------------------------------------------------------

class XpdfMenuButton: public QToolButton {
  Q_OBJECT

public:

  XpdfMenuButton(QMenu *menuA);

private slots:

  void btnPressed();

private:

  QMenu *menu;
};

//------------------------------------------------------------------------
// XpdfErrorWindow
//------------------------------------------------------------------------

class XpdfErrorWindow: public QWidget {
  Q_OBJECT

public:

  XpdfErrorWindow(XpdfViewer *viewerA, int errorEventTypeA);
  virtual ~XpdfErrorWindow();
  virtual QSize sizeHint() const;
  virtual void closeEvent(QCloseEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void customEvent(QEvent *event);

private slots:

  void clearBtnPressed();

private:

  static void errorCbk(void *data, ErrorCategory category,
		       int pos, char *msg);
  static void dummyErrorCbk(void *data, ErrorCategory category,
			    int pos, char *msg);

  XpdfViewer *viewer;
  int errorEventType;
  QListWidget *list;
  QSize lastSize;
};

//------------------------------------------------------------------------
// XpdfViewer
//------------------------------------------------------------------------

class XpdfViewer: public QMainWindow {
  Q_OBJECT

public:

  XpdfViewer(XpdfApp *appA, GBool fullScreen);

  static XpdfViewer *create(XpdfApp *app, QString fileName, int page,
			    QString destName, QString password,
			    GBool fullScreen);

  virtual ~XpdfViewer();

  virtual QSize sizeHint() const;

  void tweakSize();

  // Open a file in the current tab.  Returns a boolean indicating
  // success.
  GBool open(QString fileName, int page, QString destName,
	     QString password);

  // Open a file in a new tab.  Returns a boolean indicating success.
  GBool openInNewTab(QString fileName, int page, QString destName,
		     QString password, GBool switchToTab);

  virtual QMenu *createPopupMenu();

private slots:

  void pdfResized();
  void pdfPaintDone(bool finished);
  void preLoad();
  void postLoad();
  void keyPress(QKeyEvent *e);
  void mousePress(QMouseEvent *e);
  void mouseRelease(QMouseEvent *e);
  void mouseWheel(QWheelEvent *e);
  void mouseMove(QMouseEvent *e);
  void pageChange(int pg);
#if XPDFWIDGET_PRINTING
  void printStatus(int nextPage, int firstPage, int lastPage);
  void cancelPrint();
#endif

  void openMenuAction();
  void openInNewWinMenuAction();
  void reloadMenuAction();
  void saveAsMenuAction();
  void saveImageMenuAction();
#if XPDFWIDGET_PRINTING
  void printMenuAction();
#endif
  void quitMenuAction();
  void copyMenuAction();
  void singlePageModeMenuAction();
  void continuousModeMenuAction();
  void sideBySideSingleModeMenuAction();
  void sideBySideContinuousModeMenuAction();
  void horizontalContinuousModeMenuAction();
  void fullScreenMenuAction(bool checked);
  void rotateClockwiseMenuAction();
  void rotateCounterclockwiseMenuAction();
  void zoomToSelectionMenuAction();
  void newTabMenuAction();
  void newWindowMenuAction();
  void closeTabMenuAction();
  void closeWindowMenuAction();
  void openErrorWindowMenuAction();
  void helpMenuAction();
  void aboutMenuAction();

  void popupMenuAction(int idx);

  void pageNumberChanged();
  void backButtonPressed();
  void forwardButtonPressed();
  void zoomOutButtonPressed();
  void zoomInButtonPressed();
  void zoomIndexChanged(const QString &zoomText);
  void zoomEditingFinished();
  void fitWidthButtonPressed();
  void fitPageButtonPressed();
  void selectModeButtonPressed();
  void statusIndicatorPressed();
  void findTextChanged();
  void findNextButtonPressed();
  void findPrevButtonPressed();
  void newTabButtonPressed();

  void switchTab(QListWidgetItem *current, QListWidgetItem *previous);
  void infoComboBoxChanged(int idx);
  void outlineItemClicked(const QModelIndex& idx);
  void layerItemClicked(const QModelIndex& idx);
  void attachmentSaveClicked(int idx);

  void clearFindError();

private:

  friend class XpdfErrorWindow;

  //--- commands
  void execCmd(const char *cmd, QInputEvent *event);
  void cmdAbout(GString *args[], int nArgs, QInputEvent *event);
  void cmdBlockSelectMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdCloseTabOrQuit(GString *args[], int nArgs, QInputEvent *event);
  void cmdCloseSidebar(GString *args[], int nArgs, QInputEvent *event);
  void cmdCloseSidebarMoveResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdCloseSidebarResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdCloseWindowOrQuit(GString *args[], int nArgs, QInputEvent *event);
  void cmdContinuousMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdCopy(GString *args[], int nArgs, QInputEvent *event);
#if 0 // for debugging
  void cmdDebug1(GString *args[], int nArgs, QInputEvent *event);
#endif
  void cmdEndPan(GString *args[], int nArgs, QInputEvent *event);
  void cmdEndSelection(GString *args[], int nArgs, QInputEvent *event);
  void cmdFind(GString *args[], int nArgs, QInputEvent *event);
  void cmdFindFirst(GString *args[], int nArgs, QInputEvent *event);
  void cmdFindNext(GString *args[], int nArgs, QInputEvent *event);
  void cmdFindPrevious(GString *args[], int nArgs, QInputEvent *event);
  void cmdFocusToDocWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdFocusToPageNum(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLink(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLinkInNewTab(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLinkInNewTabNoSel(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLinkInNewWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLinkInNewWinNoSel(GString *args[], int nArgs, QInputEvent *event);
  void cmdFollowLinkNoSel(GString *args[], int nArgs, QInputEvent *event);
  void cmdFullScreenMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdGoBackward(GString *args[], int nArgs, QInputEvent *event);
  void cmdGoForward(GString *args[], int nArgs, QInputEvent *event);
  void cmdGotoDest(GString *args[], int nArgs, QInputEvent *event);
  void cmdGotoLastPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdGotoPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdHelp(GString *args[], int nArgs, QInputEvent *event);
  void cmdHorizontalContinuousMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdLinearSelectMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdNewTab(GString *args[], int nArgs, QInputEvent *event);
  void cmdNewWindow(GString *args[], int nArgs, QInputEvent *event);
  void cmdNextPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdNextPageNoScroll(GString *args[], int nArgs, QInputEvent *event);
  void cmdNextTab(GString *args[], int nArgs, QInputEvent *event);
  void cmdOpen(GString *args[], int nArgs, QInputEvent *event);
  void cmdOpenErrorWindow(GString *args[], int nArgs, QInputEvent *event);
  void cmdOpenSidebar(GString *args[], int nArgs, QInputEvent *event);
  void cmdOpenSidebarMoveResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdOpenSidebarResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdPageDown(GString *args[], int nArgs, QInputEvent *event);
  void cmdPageUp(GString *args[], int nArgs, QInputEvent *event);
  void cmdPostPopupMenu(GString *args[], int nArgs, QInputEvent *event);
  void cmdPrevPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdPrevPageNoScroll(GString *args[], int nArgs, QInputEvent *event);
  void cmdPrevTab(GString *args[], int nArgs, QInputEvent *event);
#if XPDFWIDGET_PRINTING
  void cmdPrint(GString *args[], int nArgs, QInputEvent *event);
#endif
  void cmdQuit(GString *args[], int nArgs, QInputEvent *event);
  void cmdReload(GString *args[], int nArgs, QInputEvent *event);
  void cmdRotateCW(GString *args[], int nArgs, QInputEvent *event);
  void cmdRotateCCW(GString *args[], int nArgs, QInputEvent *event);
  void cmdRun(GString *args[], int nArgs, QInputEvent *event);
  void cmdSaveAs(GString *args[], int nArgs, QInputEvent *event);
  void cmdSaveImage(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollDown(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollDownNextPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollLeft(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollOutlineDown(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollOutlineUp(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollRight(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToBottomEdge(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToBottomRight(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToLeftEdge(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToRightEdge(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToTopEdge(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollToTopLeft(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollUp(GString *args[], int nArgs, QInputEvent *event);
  void cmdScrollUpPrevPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdSetSelection(GString *args[], int nArgs, QInputEvent *event);
  void cmdSideBySideContinuousMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdSideBySideSingleMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdSinglePageMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdStartPan(GString *args[], int nArgs, QInputEvent *event);
  void cmdStartSelection(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleContinuousMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleFullScreenMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleSelectMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleSidebar(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleSidebarMoveResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdToggleSidebarResizeWin(GString *args[], int nArgs, QInputEvent *event);
  void cmdWindowMode(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomFitPage(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomFitWidth(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomIn(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomOut(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomPercent(GString *args[], int nArgs, QInputEvent *event);
  void cmdZoomToSelection(GString *args[], int nArgs, QInputEvent *event);
  int scaleScroll(int delta);
  void followLink(QInputEvent *event, GBool onlyIfNoSel,
		  GBool newTab, GBool newWindow);

  //--- GUI events
  int getModifiers(Qt::KeyboardModifiers qtMods);
  int getContext(Qt::KeyboardModifiers qtMods);
  int getMouseButton(Qt::MouseButton qtBtn);
  virtual void keyPressEvent(QKeyEvent *e);

  //--- GUI setup
  void createWindow();
  void createToolBar();
  QToolButton *addToolBarButton(const QIcon &icon,
				const char *slot, const char *tip);
  XpdfMenuButton *addToolBarMenuButton(const QIcon &icon,
				       const char *tip, QMenu *menu);
  void addToolBarSeparator();
  void addToolBarSpacing(int w);
  void addToolBarStretch();
  void createMainMenu();
  void createXpdfPopupMenu();
  QWidget *createTabPane();
  QWidget *createInfoPane();
  void updateInfoPane();
  void destroyWindow();
  void enterFullScreenMode();
  void exitFullScreenMode();
  void addTab();
  void closeTab(XpdfTabInfo *tab);
  void gotoTab(int idx);
  void updateModeInfo();
  void updateZoomInfo();
  void updateSelectModeInfo();
  void updateDocInfo();
  void updateOutline(int pg);
  void setOutlineOpenItems(const QModelIndex &idx);
  void fillAttachmentList();
  void statusIndicatorStart();
  void statusIndicatorStop();
  void statusIndicatorOk();
  void statusIndicatorError();
  void showFindError();
  void createAboutDialog();
  void execSaveImageDialog();

  static XpdfViewerCmd cmdTab[];

  XpdfApp *app;

  // menu
  QMenuBar *mainMenu;
  QMenu *displayModeSubmenu;
  QAction *fullScreenMenuItem;

  // popup menu
  QMenu *popupMenu;
  QSignalMapper *popupMenuSignalMapper;

  // toolbar
  int toolBarFontSize;		// used for HiDPI scaling
  QToolBar *toolBar;
  QLineEdit *pageNumber;
  QLabel *pageCount;
  QComboBox *zoomComboBox;
  QToolButton *fitWidthBtn;
  QToolButton *fitPageBtn;
  QToolButton *selectModeBtn;
  PropertyListAnimation *indicatorAnimation;
  QList<QVariant> indicatorIcons;
  QList<QVariant> indicatorErrIcons;
  QLineEdit *findEdit;
  QAction *findCaseSensitiveAction;
  QAction *findWholeWordsAction;

  // sidebar pane
  QSplitter *sidebarSplitter;
  int sidebarWidth;
  QListWidget *tabList;
  QComboBox *infoComboBox;
  QStackedLayout *infoStack;

  // viewer pane
  QStackedWidget *viewerStack;

  QLabel *linkTargetBar;
  QString linkTargetInfo;

  GList *tabInfo;		// [XpdfTabInfo]
  XpdfTabInfo *currentTab;

  XpdfWidget::DisplayMode fullScreenPreviousDisplayMode;
  double fullScreenPreviousZoom;

  int lastMousePressX, lastMousePressY;

  QTimer *findErrorTimer;

  XpdfErrorWindow *errorWindow;
  QDialog *aboutDialog;
#if XPDFWIDGET_PRINTING
  QProgressDialog *printStatusDialog;
#endif

  QString lastFileOpened;
};

#endif
