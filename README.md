# stec
 **S**traight
 **T**ext
 **E**ditor for
 **C**onsole
 
 A down to earth text-editor that is straight-forward, simple and has no obscure keybindings. 
 Ctrl+Z to undo. Ctrl+Y to redo. Ctrl+S to save.

# TODO
- [x] Make reading files possible
- [ ] Make typing possible
- [ ] Make saving possible
- [ ] Just make it better
- [ ] Markdown Table handler

# Goals
- Be aimed to be great for Markdown
- Be usable by someone who's never used it, especially if you aren't familiar with Linux
- Automatic resizing of Markdown tables (similar to the Advanced Tables Plugin for Obsidian)
- Work on both Windows and Linux without any conessions
- Avoid platform specific code unless absolutely necessary
- Work without any third-party libraries that most compilers or environments don't come with

# How it works:
First the loaded file is split up based on it's lines.
These lines are saved in the form of a Linked List.