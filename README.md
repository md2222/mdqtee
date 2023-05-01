mdqtee (MD QTextEdit Example) - rich text editor based on Qt QTextEdit example.

Feautures:
- Can work with files in MAFF format. All document files in one zip-file.
- In HTML-format all resource files copies to [name]_files directory.
- You can insert (attach) various files. Video, PDF, ...
- You can copy entire articles from the internet, images will be downloaded. Drag-and-drop image from browser (will download it).
- Export formats: PDF, EPUB 2, Markdown, Plain text, ODT (Qt feature, not good).
- For links click use Ctrl. Links are opened by system applications.
- Anchors.
- You can replace image with its link or thumbnail.
- Table actions. Tables can be with % column width. Use Shift+ when create it.
- [+], [-] buttons can resize selected image. One at a time. Can resize table column width for table created with % width.
- Actions with Ctrl key for scroll bar. (Scroll by pages, Scroll to top, Scroll to bottom)
- File browser panel. 
- Data file path as command line argument. It allows you to open a data file from file manager and make mdqtee as default (for maff files, for example).

Known issues: on some images the special context menu items only opens on the right half. It overlaped by the previous fragment.

![mdqtee](https://raw.githubusercontent.com/md2222/mdqtee/master/mdqtee-screenshot-01.png)
