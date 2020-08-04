# Scheme-Windows-Workspace
This is a windows workspace for Chez Scheme.

## Reason

Provides a simple alternative to a terminal; creates a workspace type interface; that prevents text rushing off the screen;  keeps results and output into a different pane than the script source.

- Uses tiled panes rather than overlapping windows; there are several standard view combinations that rearrange the panes.

- Provides a graphical output pane. 
  - In this version Direct2D is used for hardware acceleration.
  
- I use versions of this application to write apps that script dozens of C libraries; Chez Scheme is very good at that.


## Fun things
Interactively editing an animation or a 2D game while it is still running.

You can work through the online Scheme programming book; executing the examples as you go.

## Win32 Direct2D version

This is the conventional win32 version of a workspace for running scheme.

It uses the legacy (IE11) web view for documentation and Scintilla for text.

There are other variants of this idea; that are modern browser/web view integrated. 



## Requires Windows 10 64bit.

## Selfie 
<img src="assets/Selfie.png.png">

## Related Projects

There is also a Direct2D player application.

There is another version of this work-space; that uses GDI+.


## Chez Scheme

https://github.com/cisco/ChezScheme

### Win32++

Win32++  is used for the windows; dockers etc.

http://win32-framework.sourceforge.net  

### Scintilla

Scintilla editor is used for the text/editor views.
http://www.scintilla.org/

**These are all open source components**

*Any (few) modifications are in the source code of the application.* 

See the license in the documentation.
