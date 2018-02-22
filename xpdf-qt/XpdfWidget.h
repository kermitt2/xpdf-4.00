//========================================================================
//
// XpdfWidget.h
//
// Copyright 2009-2017 Glyph & Cog, LLC
//
//========================================================================

//! \mainpage
//!
//! XpdfWidget is a PDF viewer widget class for Qt.
//! <br><br><br>
//! Copyright 2009-2017 Glyph & Cog, LLC

//! \file

#ifndef XPDFWIDGET_H
#define XPDFWIDGET_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <QAbstractScrollArea>

class QMutex;
class QTimer;
#if XPDFWIDGET_PRINTING
class QPrinter;
#endif

class GString;
class PDFDoc;
class QtPDFCore;

//------------------------------------------------------------------------


/*! Opaque handle used to represent an outline node. */
typedef void *XpdfOutlineHandle;

/*! Opaque handle used to represent a layer. */
typedef void *XpdfLayerHandle;

/*! Opaque handle used to represent a layer display order tree node. */
typedef void *XpdfLayerOrderHandle;

/*! Opaque handle used to represent an annotation. */
typedef void *XpdfAnnotHandle;

/*! Opaque handle used to represent a form field. */
typedef void *XpdfFormFieldHandle;

/*! Opaque handle used to represent a PDF document. */
typedef void *XpdfDocHandle;

//------------------------------------------------------------------------

//! A PDF viewer widget class for Qt.
class XpdfWidget: public QAbstractScrollArea {
  Q_OBJECT

public:

  //! Error codes returned by certain XpdfViewer functions.
  enum ErrorCode {
    pdfOk               =    0,	//!< no error
    pdfErrOpenFile      =    1,	//!< couldn't open the PDF file
    pdfErrBadCatalog    =    2,	//!< couldn't read the page catalog
    pdfErrDamaged       =    3,	//!< PDF file was damaged and couldn't be
				//!<   repaired
    pdfErrEncrypted     =    4,	//!< file was encrypted and password was
				//!<   incorrect or not supplied
    pdfErrHighlightFile =    5,	//!< nonexistent or invalid highlight file
    pdfErrBadPrinter    =    6,	//!< invalid printer
    pdfErrPrinting      =    7,	//!< error during printing
    pdfErrPermission    =    8,	//!< PDF file doesn't allow that operation
    pdfErrBadPageNum    =    9,	//!< invalid page number
    pdfErrFileIO        =   10,	//!< file I/O error
    pdfErrNoHandle      = 1001,	//!< NULL object handle
    pdfErrOutOfMemory   = 1002,	//!< out of memory
    pdfErrBusy          = 1003,	//!< PDF component is busy
    pdfErrBadArg        = 1004	//!< invalid argument
  };

  //! Display modes, to be passed to XpdfWidget::setDisplayMode().
  enum DisplayMode {
    pdfDisplaySingle,			//!< single page
    pdfDisplayContinuous,		//!< pages stacked vertically
    pdfDisplaySideBySideSingle,		//!< two facing pages
    pdfDisplaySideBySideContinuous,	//!< facing pages, stacked vertically
    pdfDisplayHorizontalContinuous	//!< pages stacked horizontally
  };

  //! \name Zoom values
  //! Special values for XpdfWidget::setZoom() / XpdfWidget::getZoom()
  //@{
  static const int zoomToPage  = -1;	//!< zoom to fit whole page
  static const int zoomToWidth = -2;	//!< zoom to fit page width
  static const int zoomToHeight = -3;	//!< zoom to fit page height
  //@}

  //! \name Find flags
  //! Flags to be passed to XpdfWidget::find()
  //@{
  //! search backward from the starting point
  static const int findBackward      = 0x00000001;
  //! perform a case-sensitive search (default is case-insensitive)
  static const int findCaseSensitive = 0x00000002;
  //! start searching from the previous search result
  static const int findNext          = 0x00000004;
  //! limit the search to the current page
  static const int findOnePageOnly   = 0x00000008;
  //! limit the search to whole words
  static const int findWholeWord     = 0x00000010;
  //@}

  //! Initialize the XpdfWidget class, reading a configuration file.
  //! If \a configFileName is non-empty, the specified file is
  //! tried first.  If \a configFileName is empty, or the file
  //! doesn't exist, the default location is tried (\c <exe-dir>/xpdfrc
  //! on Windows; \c ~/\c .xpdfrc on Unix).
  //!
  //! This function must be called before any other XpdfWidget functions
  //! (including the constructor).  It will be called automatically
  //! (with a NULL \a configFileName) if it hasn't already been
  //! called when the first XpdfWidget function is used.
  static void init(const QString &configFileName = QString());

  //! Process a configuration command, i.e., one line of an xpdfrc file.
  //! Note that this applies globally to all XpdfWidget instances.
  static void setConfig(const QString &command);

  //! The XpdfWidget constructor.
  //! \param paperColor the paper background color (which should generally
  //!        be left as white)
  //! \param matteColor the matte color displayed between pages, and around
  //!        pages that are smaller than the window
  //! \param reverseVideo sets reverse video at startup
  //! \param parentA the parent QWidget
  XpdfWidget(const QColor &paperColor = QColor(0xff, 0xff, 0xff),
	     const QColor &matteColor = QColor(0x80, 0x80, 0x80),
	     bool reverseVideo = false, QWidget *parentA = 0);

  //! The XpdfWidget constructor.
  //! \param paperColor the paper background color (which should generally
  //!        be left as white)
  //! \param matteColor the matte color displayed between pages, and around
  //!        pages that are smaller than the window
  //! \param reverseVideo sets reverse video at startup
  //! \param parentA the parent QWidget
  //! This version has the \a parent argument first so it works correctly
  //! with Qt Designer.
  XpdfWidget(QWidget *parentA,
	     const QColor &paperColor = QColor(0xff, 0xff, 0xff),
	     const QColor &matteColor = QColor(0x80, 0x80, 0x80),
	     bool reverseVideo = false);

  //! Destroys the XpdfWidget.
  virtual ~XpdfWidget();

  //! Control handling of hyperlinks.
  //! If enabled, the viewer will follow hyperlinks when clicked with the
  //! left mouse button.  If disabled, the viewer will ignore left mouse
  //! button clicks on hyperlinks.  The default is enabled.
  void enableHyperlinks(bool on);

  //! Control handling of text selection.
  //! If enabled, the viewer will allow the user to select rectangular
  //! regions of text when the user drags with the left mouse button.  If
  //! disabled, dragging with the left mouse button is ignored.  The
  //! default is enabled.
  void enableSelect(bool on);

  //! Control mouse panning.
  //! If enabled, dragging with the middle mouse button pressed will pan
  //! the page.  If disabled, the middle button is ignored.  The default
  //! is enabled.
  void enablePan(bool on);

  //! Control keypress passthrough.
  //! If enabled, XpdfWidget will pass keypress events through to the
  //! keyPress signal, with no other processing.  If disabled, XpdfWidget
  //! will implement some built-in key bindings.  The default is disabled.
  void setKeyPassthrough(bool on) { keyPassthrough = on; }

  //! Control mouse event passthrough.
  //! If enabled, XpdfWidget will pass mouse events through to the
  //! mousePress/mouseRelease signals, with no other processing.  If
  //! disabled, XpdfWidget will implement some built-in mouse handling
  //! (in addition to sending the signals).  The default is disabled.
  void setMousePassthrough(bool on) { mousePassthrough = on; }

  //! Control the password dialog.
  //! If enabled, the viewer will show a password dialog for encrypted
  //! files; if disabled, it will simply return \c pdfErrEncrypted unless
  //! the correct password is passed to \c pdfLoadFileWithPassword.  The
  //! default is enabled.
  void showPasswordDialog(bool showDlg);

  //! Set the matte color, i.e., the color used for background outside
  //! the actual page area.  The default is a medium gray.
  void setMatteColor(const QColor &matteColor);

  //! Turn reverse video mode on/off.  The default is off.
  void setReverseVideo(bool reverse);

  //! Set the cursor.  The requested cursor will only be displayed in
  //! the viewport (not in the scrollbars).
  void setCursor(const QCursor &cursor);

  //! Reset to the default cursor.
  void unsetCursor();

  //! Load a PDF file and display its first page.
  //! \param fileName the PDF file to load
  //! \param password a string to be tried first as the owner password
  //!        and then as the user password
  //! \return \c pdfOk if successful; an error code, otherwise
  ErrorCode loadFile(const QString &fileName,
		     const QString &password = "");

  //! Load a PDF file from a memory buffer and display its first page.
  //! \param buffer the PDF file in memory
  //! \param bufferLength length of \a buffer
  //! \param password a string to be tried first as the owner password
  //!        and then as the user password
  //! \return \c pdfOk if successful; an error code otherwise
  ErrorCode loadMem(const char *buffer, unsigned int bufferLength,
		    const QString &password = "");

  //! Load a PDF file and return a handle.

  //! This function can be safely called from a non-GUI thread.  Use
  //! XpdfWidget::loadDoc (on the GUI thread) to load the document
  //! handle into the viewer.  The handle returned in *\c docPtr
  //! should be passed to either XpdfWidget::loadDoc or
  //! XpdfWidget::freeDoc.
  //!
  //! Calling XpdfWidget::readDoc + XpdfWidget::loadDoc is equivalent
  //! to calling XpdfWidget::loadFile.  The difference is that readDoc
  //! can be called on a background thread to avoid stalling the user
  //! interface.
  //! \param docPtr the PDF document handle will be returned her
  //! \param fileName the PDF file to load
  //! \param password a string to be tried first as the owner password
  //!        and then as the user password
  //! \return \c pdfOk if successful; an error code, otherwise
  ErrorCode readDoc(XpdfDocHandle *docPtr,
		    const QString &fileName,
		    const QString &password = "");

  //! Load a PDF document and display its first page.
  //! This function displays a PDF document handle created by
  //! XpdfWidget::readDoc.  The document handle should not be used for
  //! anything else after calling this function.
  //! \return \c pdfOk if successful; an error code, otherwise
  ErrorCode loadDoc(XpdfDocHandle doc);

  //! Free a PDF document.
  //! This function frees a PDF document handle created by
  //! XpdfWidget::readDoc.  It should only be called if the document
  //! is not going to be displayed.  That is: after calling
  //! XpdfWidget::readDoc, you should call either XpdfWidget::loadDoc
  //! or XpdfWidget::freeDoc.  The document handle should not be used
  //! for anything else after calling this function.
  void freeDoc(XpdfDocHandle doc);

  //! Reload the current PDF file.
  //! This reloads the current PDF file, maintaining the zoom and
  //! scroll position (if possible).  This only works if the PDF file
  //! was loaded from a file (i.e., with XpdfWidget::loadFile, not
  //! with XpdfWidget::loadMem).
  //! \return \c pdfOk if successful; an error code otherwise
  ErrorCode reload();

  //! Close the currently open PDF file (if any).
  //! Calling this function is optional - the current PDF file will be
  //! automatically closed if XpdfWidget::loadFile or XpdfWidget::loadMem
  //! is called.
  void closeFile();

  //! Save the PDF file with another name.
  //! \param fileName the file to be written
  //! \return \c pdfOk if successful; an error code otherwise
  ErrorCode saveAs(const QString &fileName);

  //! Get the file name of the currently open PDF file.
  QString getFileName() const;

  //! Returns true if there is currently a PDF file open.
  bool hasOpenDocument() const;

  //! Return the number of pages in the currently open PDF file.
  //! Returns -1 if no file is open.
  int getNumPages() const;

  //! Return the currently displayed page number.
  //! Returns -1 if no file is open.
  int getCurrentPage() const;

  //! Return the page number corresponding to the middle of the
  //! window.
  int getMidPage() const;

  //! Display the specified page.
  void gotoPage(int pageNum);

  //! Display the first page.
  //! This is equivalent to \code
  //!     gotoPage(1)
  //! \endcode
  void gotoFirstPage();

  //! Display the last page.
  //! This is equivalent to \code
  //!     gotoPage(getNumPages())
  //! \endcode
  void gotoLastPage();

  //! Display the next page.
  void gotoNextPage(bool scrollToTop = true);

  //! Display the previous page.
  void gotoPreviousPage(bool scrollToTop = true);

  //! Go to a named destination.
  bool gotoNamedDestination(const QString &dest);

  //! Go forward along the history list.
  void goForward();

  //! Go backward along the history list.
  void goBackward();

  //! Scroll one screen up.
  void scrollPageUp();

  //! Scroll one screen down.
  void scrollPageDown();

  //! Scroll the page so that the top-left corner of the window is
  //! (\a xx,\a yy) pixels from the top-left corner of the PDF page.
  void scrollTo(int xx, int yy);

  //! Scroll the page by (\a dx,\a dy) pixels.  If \a dx is positive,
  //! scrolls right; if \a dx is negative, scrolls left.  Similarly,
  //! positive and negative values of \a dy scroll down and up,
  //! respectively.
  void scrollBy(int dx, int dy);

  //! Return the current scroll position x coordinate.
  int getScrollX() const;

  //! Return the current scroll position y coordinate.
  int getScrollY() const;

  //! Change the zoom factor.
  //! This can be a percentage factor (where 100 means 72 dpi) or one of
  //! the special values, XpdfWidget::zoomToPage or XpdfWidget::zoomToWidth.
  void setZoom(double zoom);

  //! Return the current zoom factor.
  //! This can be a percentage factor or one of the special values,
  //! XpdfWidget::zoomToPage or XpdfWidget::zoomToWidth.
  double getZoom() const;

  //! Return the current zoom factor as a percentage.
  //! If the zoom is set to XpdfWidget::zoomToPage or
  //! XpdfWidget::zoomToWidth, returns the computed zoom percentage
  //! for the specified page, based on the current window size.
  double getZoomPercent(int page = 1) const;

  //! Zoom in to the specified rectangle.
  //! The coordinate system is the same one used by
  //! XpdfWidget::getCurrentSelection.  This function will set the zoom
  //! factor and scroll position so that the specified rectangle just fits
  //! in the window.
  void zoomToRect(int page, double xMin, double yMin,
		  double xMax, double yMax);

  //! Set the zoom factor, while maintaining the current center.
  //! Accepts the same zoom values as XpdfWidget::setZoom.
  void zoomCentered(double zoom);

  //! Zoom so that the current page(s) fill the window width.
  //! Maintains the vertical center.
  void zoomToCurrentWidth();

  //! Change the page rotation.
  //! \param rotate rotation angle in degrees - must be 0, 90, 180, or 270
  void setRotate(int rotate);

  //! Return the current page rotation.
  //! The angle can be 0, 90, 180, or 270.
  int getRotate() const;

  //! Set continuous or single-page view mode.
  //! Deprecated: this is equivalent to calling setDisplayMode() with
  //! pdfDisplaySingle or pdfDisplayContinuous.
  //! \param continuous true for continous view mode, false for single-page
  //!        view mode
  void setContinuousMode(bool continuous);

  //! Return true if the viewer is in continuous view mode, or false
  //! if it is in any other mode.  Deprecated: see getDisplayMode().
  bool getContinuousMode() const;

  //! Set the display mode.
  void setDisplayMode(DisplayMode mode);

  //! Return the current display mode.
  DisplayMode getDisplayMode();

  //! Returns true if the mouse is currently over a hyperlink.
  bool mouseOverLink();

  //! Returns true if the specified coordinates are inside a
  //! hyperlink.  Note: This function expects PDF coordinates, not window
  //! coordinates.
  bool onLink(int page, double xx, double yy);

  //! Get destination information for the hyperlink at the specified
  //! page and coordinates.  If there is a link at the specified
  //! point, returns a string suitable for displaying to a user;
  //! otherwise returns an empty string.  Note: This function expects
  //! PDF coordinates, not window coordinates.
  QString getLinkInfo(int page, double xx, double yy);

  //! Get destination information for the hyperlink under the mouse.
  //! If the mouse is currently over a hyperlink, return an info
  //! string (same as with getLinkInfo()); otherwise return an empty
  //! string.
  QString getMouseLinkInfo();

  //! Activate the link (if any) at the specified page and coordinates.
  //! Returns true if successful.  Note: This function expects PDF
  //! coordinates, not window coordinates.
  bool gotoLinkAt(int page, double xx, double yy);

  //! Check for an annotation containing the specified point.
  //! Returns NULL if there is no annotation at this point.  Note:
  //! This function expects PDF coordinates, not window coordinates.
  XpdfAnnotHandle onAnnot(int page, double xx, double yy);

  //! Get the annotation type.
  QString getAnnotType(XpdfAnnotHandle annot);

  //! Get the annotation's content.
  //! Usage of this depends on the annotation type.
  QString getAnnotContent(XpdfAnnotHandle annot);

  //! Check for a form field containing the specified point.
  //! Returns NULL if there is no annotation at this point.  Note:
  //! This function expects PDF coordinates, not window coordinates.
  XpdfFormFieldHandle onFormField(int page, double xx, double yy);

  //! Get the form field's type.
  QString getFormFieldType(XpdfFormFieldHandle field);

  //! Get the form field's name.
  QString getFormFieldName(XpdfFormFieldHandle field);

  //! Get the form field's content.
  //! Usage of this depends on the field type.
  QString getFormFieldValue(XpdfFormFieldHandle field);

  //! Get the form field's bounding box.
  void getFormFieldBBox(XpdfFormFieldHandle field, int *pageNum,
			double *xMin, double *yMin,
			double *xMax, double *yMax);

  //! Convert window coordinates to PDF coordinates.  Returns true if
  //! successful, i.e., if the specified point falls on a PDF page.
  bool convertWindowToPDFCoords(int winX, int winY,
				int *page, double *pdfX, double *pdfY);

  //! Convert PDF coordinates to window coordinates.
  void convertPDFToWindowCoords(int page, double pdfX, double pdfY,
				int *winX, int *winY);

  //! Enable or disable window redraws.
  //! This is useful, e.g., for avoiding extra redraws during window
  //! resizing.  Deprecated -- this just calls setUpdatesEnabled().
  void enableRedraw(bool enable);

  //! Return the coordinates of the specified page box.
  //! \param page the page number
  //! \param box the requested page box - one of "media", "crop",
  //!        "bleed", "trim", or "art" (\a box is not case-sensitive)
  //! \param *xMin returns the minimum x coordinate of the box
  //! \param *yMin returns the minimum y coordinate of the box
  //! \param *xMax returns the maximum x coordinate of the box
  //! \param *yMax returns the maximum y coordinate of the box
  //!
  //! All coordinates are in points (1 point = 1/72 inch).
  void getPageBox(int page, const QString &box,
		  double *xMin, double *yMin, double *xMax, double *yMax) const;

  //! Return the width of the specified page.
  //! This function returns the crop box width, measured in points
  //! (1 point = 1/72 inch).
  double getPageWidth(int page) const;

  //! Return the height of the specified page.
  //! This function returns the crop box height, measured in points
  //! (1 point = 1/72 inch).
  double getPageHeight(int page) const;

  //! Get the default rotation for the specified page.
  //! This is the default viewing rotation specified in the PDF file -
  //! it will be one of 0, 90, 180, or 270.
  int getPageRotation(int page) const;

  //! Check to see if there is a selection.
  //! Returns true if there is a currently active selection.
  bool hasSelection();

  //! Returns the current selection.
  //! If there is a currently active selection, sets *\a page, (*\a x0,*\a y0),
  //! and (*\a x1,*\a y1) to the page number and upper-left and lower-right
  //! coordinates, respectively, and returns true.  If there is no selection,
  //! returns false.
  bool getCurrentSelection(int *page, double *x0, double *y0,
			   double *x1, double *y1) const;

  //! Set the selection.
  //! Sets the current selection to the rectangle with upper-left corner
  //! (\a x0,\a y0) and lower-right corner (\a x1,\a y1) on \a page.
  void setCurrentSelection(int page, double x0, double y0,
			   double x1, double y1);

  //! Clear the selection.
  void clearSelection();

  //! Check for block selection mode.
  //! Returns true if the current selection mode is block.
  bool isBlockSelectMode();

  //! Check for linear selection mode.
  //! Returns true if the current selection mode is linear.
  bool isLinearSelectMode();

  //! Set block selection mode.
  //! In this mode, the selection is a simple rectangle.  Any part of
  //! the page can be selected, regardless of the content on the page.
  void setBlockSelectMode();

  //! Set linear selection mode.
  //! In this mode, the selection follows text.  Non-text regions
  //! cannot be selected.
  void setLinearSelectMode();

  //! Set the selection color.
  void setSelectionColor(const QColor &selectionColor);


  //! Force a complete redraw.
  void forceRedraw();

#if XPDFWIDGET_PRINTING
  //! Checks to see if printing is allowed.
  //! This function returns false if the currently displayed PDF file
  //! is encrypted and does not allow printing (or if no PDF file is
  //! currently open).  The owner password can be used to circumvent
  //! this: if a valid owner password was supplied to
  //! XpdfWidget::loadFile, this function will always return true.  If
  //! this function returns false, the printing functions will return
  //! an error.
  bool okToPrint() const;

  //! Print the currently displayed PDF file.
  //! Prints the currently displayed PDF file.  If \a showDialog is
  //! true, displays the Qt print dialog, and uses the printer
  //! selected by the user.  If \a showDialog is false, prints to the
  //! default printer without showing any dialogs.
  ErrorCode print(bool showDialog);

  //! Print the currently displayed PDF file.
  //! Prints the currently displayed PDF file to \a prt.
  ErrorCode print(QPrinter *prt);

  //! Cancel an in-progress print job.  This should be called in
  //! response to a printStatus signal.
  void cancelPrint() { printCanceled = true; }

  void updatePrintStatus(int nextPage, int firstPage, int lastPage);
  bool isPrintCanceled() { return printCanceled; }

  //! Set the horizontal and vertical print resolution, in dots per
  //! inch (DPI).  The horizontal and vertical resolutions are
  //! typically the same.  (There are exceptions, such as some chart
  //! printers.)
  void setPrintDPI(int hDPI, int vDPI);
#endif // XPDFWIDGET_PRINTING

  //! Convert a page to a color image.
  //! This function converts the page number \a page to a 24-bit RGB
  //! bitmap, at a resolution of \a dpi dots per inch.
  QImage convertPageToImage(int page, double dpi);

  //! Convert a rectangular region of a page to a color image.
  //! This function converts a rectangular region, defined by
  //! corners (\a x0,\a y0) and (\a x1,\a y1), of page number \a page
  //! to a 24-bit RGB bitmap, at a resolution of \a dpi dots per inch.
  QImage convertRegionToImage(int page, double x0, double y0,
			      double x1, double y1, double dpi);

  //! Retrieve an embedded thumbnail image.
  //! This function returns the embedded thumbnail image for the
  //! specified page, or a null image if there is no embedded
  //! thumbnail.  This function does not do any rasterization -- it
  //! only returns a non-null image if there is an embedded thumbnail
  //! in the PDF file.
  QImage getThumbnail(int page);

  //! Checks to see if text extraction is allowed.
  //! This function returns false if the currently displayed PDF file
  //! is encrypted and does not allow extraction of text (or if no PDF
  //! file is currently open).  The owner password can be used to
  //! circumvent this: if a valid owner password was supplied to
  //! XpdfWidget::loadFile, this function will always return true.
  //! If this function returns false, the text extraction functions will
  //! not return any text.
  bool okToExtractText() const;

  //! Set the encoding to use for text extraction.
  //! The following encodings are predefined:
  //!   - \c "Latin1": ISO-8859-1 (this is the default value)
  //!   - \c "ASCII7": 7-bit ASCII
  //!   - \c "UTF-8": Unicode in UTF-8 format
  //!   - \c "UCS-2": Unicode in UCS-2 format
  //!
  //! Additional encodings can be defined via the xpdfrc config file.
  void setTextEncoding(const QString &encodingName);

  //! Extract text from a region of a page.
  //! This function extracts and returns text from the rectangular
  //! region, defined by corners (\a x0,\a y0) and (\a x1,\a y1), of
  //! page number \a page.  The coordinates returned by
  //! XpdfWidget::getCurrentSelection may be passed directly to this
  //! function.  Returns an empty string if no file is open or if
  //! text extraction is not allowed.
  QString extractText(int page, double x0, double y0,
		      double x1, double y1);

  //! Get the currently selected text.
  //! Returns an empty string if there is no selection (or if there is
  //! no text in the selected region).
  QString getSelectedText();

  //! Copy the current selection to the clipboard.
  void copySelection();

  //! Find a text string.
  //! This function searches for a Unicode text string.  Starts
  //! searching after (before, if searching backward) the current
  //! selection (if there is a selection), or at the top (bottom,
  //! if searching backward) of the current page (if there is no
  //! selection).  The \a flags argument consists of zero or more
  //! of the following, or-ed together:
  //!   - \c findBackward - search backward from the starting point
  //!   - \c findCaseSensitive - perform a case-sensitive search
  //!     (default is case-insensitive)
  //!   - \c findNext - start searching from the previous search result
  //!   - \c findOnePageOnly - limit the search to the current page
  //!   - \c findWholeWord - limit the search to whole words
  bool find(const QString &text, int flags = 0);

  //! Return the number of children of an outline tree node.
  //! This function returns the number of children of node \a outline,
  //! or the number of root outline entries if \a outline is \c NULL.
  int getOutlineNumChildren(XpdfOutlineHandle outline);

  //! Return a child of an outline tree node.
  //! This function returns the \a idx 'th child of node \a outline,
  //! or the \a idx 'th root entry if \a outline is \c NULL.
  XpdfOutlineHandle getOutlineChild(XpdfOutlineHandle outline, int idx);

  //! Return the parent of an outline tree node.
  //! This function returns the parent of node \a outline, or NULL if
  //! \a outline is a root item.
  XpdfOutlineHandle getOutlineParent(XpdfOutlineHandle outline);

  //! Get the title of an outline tree node.
  //! This function returns the title of node \a outline.
  QString getOutlineTitle(XpdfOutlineHandle outline);

  //! Return true if the specified outline entry starts open.
  bool getOutlineStartsOpen(XpdfOutlineHandle outline);

  //! Return the target page number for the specified outline entry.
  int getOutlineTargetPage(XpdfOutlineHandle outline);

  //! Jump to the target of the specified outline entry.
  void gotoOutlineTarget(XpdfOutlineHandle outline);

  //! Return the number of layers in the PDF file.
  //! Note that a PDF file can have zero or more layers.
  int getNumLayers() const;

  //! Get a layer handle.
  //! This function returns a handle for the \a idx 'th layer.
  XpdfLayerHandle getLayer(int idx) const;

  //! Get the name of a layer.
  //! This function returns the title of \a layer.
  QString getLayerName(XpdfLayerHandle layer) const;

  //! Get the visibility state of a layer.
  //! Returns true if the layer is currently visible, false if not.
  bool getLayerVisibility(XpdfLayerHandle layer) const;

  //! Set the visibility state of a layer.
  //! \param layer the layer handle
  //! \param visibility the new state - true for visible, false for not
  //!        visible
  void setLayerVisibility(XpdfLayerHandle layer, bool visibility);

  //! Get the suggested state for this layer in viewing mode.
  //! This function returns one of:
  //!   -  1: on
  //!   -  0: off
  //!   - -1: unset
  int getLayerViewState(XpdfLayerHandle layer) const;

  //! Get the suggested state for this layer in printing mode.
  //! This function returns one of:
  //!   -  1: on
  //!   -  0: off
  //!   - -1: unset
  int getLayerPrintState(XpdfLayerHandle layer) const;

  //! Get the root of the layer display order tree.
  XpdfLayerOrderHandle getLayerOrderRoot() const;

  //! Check the type of a layer display order tree node.
  //! Returns true if the specified node of the layer display order
  //! tree is a name; false if the node is a layer.
  bool getLayerOrderIsName(XpdfLayerOrderHandle order) const;

  //! Get the name of a layer display order tree node.
  //! This should only be called if getLayerOrderIsName returns true.
  QString getLayerOrderName(XpdfLayerOrderHandle order) const;

  //! Get the layer associated with a layer display order tree node.
  XpdfLayerHandle getLayerOrderLayer(XpdfLayerOrderHandle order);

  //! Returns the number of children attached to a layer display order
  //! tree node.
  int getLayerOrderNumChildren(XpdfLayerOrderHandle order);

  //! Returns the \a idx 'th child of a layer display order tree node.
  XpdfLayerOrderHandle getLayerOrderChild(XpdfLayerOrderHandle order, int idx);

  //! Return the parent of a layer display order tree node.
  //! This function returns the parent of node \a order, or NULL if \a
  //! order is the root node.
  XpdfLayerOrderHandle getLayerOrderParent(XpdfLayerOrderHandle order);

  //! Return the number of embedded files in the current PDF file.
  int getNumEmbeddedFiles();

  //! Return the name of the \a idx 'th embedded file.
  QString getEmbeddedFileName(int idx);

  //! Save the \a idx 'th embedded file with the specified file name.
  //! Returns true if successful.
  bool saveEmbeddedFile(int idx, QString fileName);

  //--- for internal use

  //! \cond

  virtual QSize sizeHint() const;
  QtPDFCore *getCore() { return core; }

  //! \endcond

signals:

  //! This signal is emitted whenever the viewer displays a new page.
  //! It can be triggered by user actions (e.g., the PageDown button),
  //! or program control (e.g., the gotoNextPage function).
  //! \param pageNum - the new page number
  void pageChange(int pageNum);

  //! This signal is emitted whenever the page shown at the middle of
  //! the window changes.
  //! It is similar to XpdfWidget::pageChange, except that it reflects
  //! the page shown at the middle of the window (instead of the page
  //! at the top of the window).
  void midPageChange(int pageNum);

  //! This signal is emitted just before a PDF file is loaded.
  void preLoad();

  //! This signal is emitted just after a PDF file is loaded.
  void postLoad();

  //! This signal is emitted whenever a key is pressed.
  void keyPress(QKeyEvent *e);

  //! This signal is emitted whenever a mouse button is pressed.
  void mousePress(QMouseEvent *e);

  //! This signal is emitted whenever a mouse button is released.
  void mouseRelease(QMouseEvent *e);

  //! This signal is emitted whenever the mouse pointer is moved.
  void mouseMove(QMouseEvent *e);

  //! This signal is emitted whenever a mouse wheel is clicked.
  void mouseWheel(QWheelEvent *e);

  //! This signal is emitted whenever the user clicks on a hyperlink.
  //! \param linkType the type of link - one of:
  //!          - \c "goto": a link to another page in the same PDF
  //!            file - \a dest is empty; \a page is the destination
  //!            page number
  //!          - \c "pdf": a link to another PDF file - \a dest is the
  //!            target PDF file; \a page is 0
  //!          - \c "launch": an arbitrary command to be run - \a dest
  //!            is the command; \a page is 0
  //!          - \c "url": a URL link - \a dest is the URL; \a page is 0
  //!          - \c "named": a "named action" link - \a dest is the
  //!            action (see the PDF spec for details); \a page is 0
  //!          - \c "unknown": an unknown link type - \a dest is empty;
  //!            \a page is 0
  //! \param dest destination string
  //! \param page destination page number
  void linkClick(const QString &linkType, const QString &dest, int page);

  //! This signal is emitted when the user selects an area.
  //! Use XpdfWidget::getCurrentSelection to retrieve the selection.
  void selectDone();

  //! This signal is emitted whenever the widget is repainted.  \a
  //! finished is true if the painted view is complete, or false if
  //! this was an incremental update, i.e., if the view is still being
  //! rendered.
  void paintDone(bool finished);

  //! This signal is emitted when the widget is resized.
  void resized();


#if XPDFWIDGET_PRINTING
  //! This signal is called before each page is spooled, and after the
  //! last page is spooled.  It is typically used to update a print
  //! status dialog.  \a nextPage is the next page to be printed.
  //! \a firstPage and \a lastPage specify the range of pages being
  //! printed.
  void printStatus(int nextPage, int firstPage, int lastPage);
#endif

  //! \cond PROTECTED

  void tileDone();

  //! \endcond

protected:

  //! \cond PROTECTED

  virtual void paintEvent(QPaintEvent *eventA);
  virtual void resizeEvent(QResizeEvent *eventA);
  virtual void scrollContentsBy(int dx, int dy);
  virtual void keyPressEvent(QKeyEvent *e);
  virtual void mousePressEvent(QMouseEvent *e);
  virtual void mouseReleaseEvent(QMouseEvent *e);
  virtual void mouseMoveEvent(QMouseEvent *e);
  virtual void wheelEvent(QWheelEvent *e);

  //! \endcond

private slots:

  void tick();

private:

  void setup(const QColor &paperColor, const QColor &matteColor,
	     bool reverseVideo);
  static void updateCbk(void *data, GString *fileName,
			int pageNum, int numPages,
			const char *linkLabel);
  static void midPageChangedCbk(void *data, int pageNum);
  static void preLoadCbk(void *data);
  static void postLoadCbk(void *data);
  static void linkCbk(void *data, const char *type,
		      const char *dest, int page);
  static void selectDoneCbk(void *data);
  static void paintDoneCbk(void *data, bool finished);
  static void tileDoneCbk(void *data);

  friend class XpdfViewer;
  bool getLinkTarget(int page, double xx, double yy,
		     QString &targetFileName, int &targetPage,
		     QString &targetDest);

  int printHDPI, printVDPI;
  bool printCanceled;

  static QMutex initMutex;

  QtPDFCore *core;

  bool keyPassthrough;
  bool mousePassthrough;

  QTimer *tickTimer;
};

#endif
