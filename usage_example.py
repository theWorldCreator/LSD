from Tkinter import *
import lsd
from math import sqrt
from tkFileDialog import askopenfilename
import Image, ImageTk

if __name__ == "__main__":
    root = Tk()

    #setting up a tkinter canvas with scrollbars
    frame = Frame(root, bd=2, relief=SUNKEN)
    frame.grid_rowconfigure(0, weight=1)
    frame.grid_columnconfigure(0, weight=1)
    xscroll = Scrollbar(frame, orient=HORIZONTAL)
    xscroll.grid(row=1, column=0, sticky=E+W)
    yscroll = Scrollbar(frame)
    yscroll.grid(row=0, column=1, sticky=N+S)
    canvas = Canvas(frame, bd=0, xscrollcommand=xscroll.set, yscrollcommand=yscroll.set)
    canvas.grid(row=0, column=0, sticky=N+S+E+W)
    xscroll.config(command=canvas.xview)
    yscroll.config(command=canvas.yview)
    frame.pack(fill=BOTH,expand=1)

    #adding the image
    File = askopenfilename(title = "Choose an PGM image")
    print File
    lsd_im = lsd.lsd(File)
    img = ImageTk.PhotoImage(Image.open(File))
    canvas.create_image(0,0,image=img,anchor="nw")
    canvas.config(scrollregion=canvas.bbox(ALL))

    #functions to be called when mouse is clicked
    def gradient(event):
        print "Gradient of " + str((event.x,event.y)) + " is " + str(lsd_im.grad(event.x,event.y))
    
    def perpendicular_width(x1, y1, x2, y2):
        scalar = x1 * x2 + y1 * y2
        sq_width_a = x1*x1 + y1*y1
        sq_width_b = x2*x2 + y2*y2
        
        return sqrt(sq_width_a - scalar * scalar / sq_width_b)  # trigonometry...
    
    def erase_NFA_points():
        global NFA_points
        for p in NFA_points:
            canvas.delete(p[1])
        NFA_points = []
    
    def erase_NFA_pict():
        global NFA_lines
        erase_NFA_points()
        for l in NFA_lines:
            canvas.delete(l)
        NFA_lines = []
    
    
    NFA_points = []
    NFA_lines = []
    def reset_NFA(event):
        erase_NFA_pict()
        
    def NFA(event):
        global NFA_points, NFA_lines
        if len(NFA_points) < 3:
            x = event.x
            y = event.y
            rad = 4
            circle = canvas.create_oval(x-rad,y-rad,x+rad,y+rad,width=0,fill='blue')
            NFA_points.append( [(x,y), circle] )
            
            
            if len(NFA_points) == 2:
                x1 = NFA_points[0][0][0]
                y1 = NFA_points[0][0][1]
                x2 = NFA_points[1][0][0]
                y2 = NFA_points[1][0][1]
                line = canvas.create_line(x1, y1, x2, y2, width = 1.0)
                NFA_lines.append(line)
                NFA = lsd_im.NFA(x1, y1, x2, y2, 1.0)
                print "NFA for rectangle (%f, %f), (%f, %f) with width 1.0 pix is %f" % (x1, y1, x2, y2, NFA)
            if len(NFA_points) == 3:
                x1 = NFA_points[0][0][0]
                y1 = NFA_points[0][0][1]
                x2 = NFA_points[1][0][0]
                y2 = NFA_points[1][0][1]
                x = NFA_points[2][0][0]
                y = NFA_points[2][0][1]
                width = perpendicular_width(x - x1, y - y1, x2 - x1, y2 - y1) * 2
                
                erase_NFA_points()
                canvas.delete(NFA_lines.pop())
                
                line = canvas.create_line(x1, y1, x2, y2, width = width, )
                NFA_lines.append(line)
                NFA = lsd_im.NFA(x1, y1, x2, y2, width)
                print "NFA for rectangle (%f, %f), (%f, %f) with width %d pix is %f" % (x1, y1, x2, y2, width, NFA)
                
                
    #mouseclick event
    canvas.bind("<Button 1>", gradient)
    canvas.bind("<Button 2>",reset_NFA)
    canvas.bind("<Button 3>",NFA)

    root.mainloop()
