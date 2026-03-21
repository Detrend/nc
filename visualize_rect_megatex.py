import ast
import sys
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from matplotlib.widgets import Button
from collections import defaultdict


def load_rectangles(path):
    rects = []

    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            id_part, rect_part = line.split(":", 1)
            rid = int(id_part)
            rect = ast.literal_eval(rect_part)

            rects.append((rid, rect))

    return rects


class RectangleViewer:

    def __init__(self, rectangles):

        self.fig, self.ax = plt.subplots()
        plt.subplots_adjust(bottom=0.15)

        self.patches = []
        self.patch_ids = []
        self.id_to_patches = defaultdict(list)

        for rid, (x, y, w, h) in rectangles:

            r = Rectangle(
                (x, y),
                w,
                h,
                edgecolor="black",
                facecolor="white",
                linewidth=0.8
            )

            self.ax.add_patch(r)

            self.patches.append(r)
            self.patch_ids.append(rid)
            self.id_to_patches[rid].append(r)

        self.highlighted_id = None

        self.ax.set_aspect("equal")
        self.ax.autoscale()

        self.base_xlim = self.ax.get_xlim()
        self.base_ylim = self.ax.get_ylim()

        # ID display label
        self.id_label = self.fig.text(
            0.02, 0.97,
            "ID: None",
            ha="left",
            va="top",
            fontsize=12,
            bbox=dict(facecolor="white", edgecolor="black")
        )

        # reset button
        ax_reset = plt.axes([0.85, 0.02, 0.1, 0.06])
        self.btn_reset = Button(ax_reset, "Reset")
        self.btn_reset.on_clicked(self.reset_view)

        self.dragging = False
        self.last_mouse = None

        self.fig.canvas.mpl_connect("motion_notify_event", self.on_move)
        self.fig.canvas.mpl_connect("scroll_event", self.on_scroll)
        self.fig.canvas.mpl_connect("button_press_event", self.on_press)
        self.fig.canvas.mpl_connect("button_release_event", self.on_release)

    def reset_view(self, event=None):

        self.ax.set_xlim(self.base_xlim)
        self.ax.set_ylim(self.base_ylim)
        self.fig.canvas.draw_idle()

    def on_press(self, event):

        if event.button == 3:
            self.dragging = True
            self.last_mouse = (event.xdata, event.ydata)

    def on_release(self, event):

        if event.button == 3:
            self.dragging = False

    def on_move(self, event):

        if event.inaxes != self.ax:
            return

        # PAN
        if self.dragging and event.xdata and event.ydata:

            dx = event.xdata - self.last_mouse[0]
            dy = event.ydata - self.last_mouse[1]

            x0, x1 = self.ax.get_xlim()
            y0, y1 = self.ax.get_ylim()

            self.ax.set_xlim(x0 - dx, x1 - dx)
            self.ax.set_ylim(y0 - dy, y1 - dy)

            self.last_mouse = (event.xdata, event.ydata)
            self.fig.canvas.draw_idle()
            return

        # detect rectangle under cursor
        found_id = None

        for p, rid in zip(self.patches, self.patch_ids):
            contains, _ = p.contains(event)
            if contains:
                found_id = rid
                break

        if found_id != self.highlighted_id:

            # restore previous group
            if self.highlighted_id is not None:
                for p in self.id_to_patches[self.highlighted_id]:
                    p.set_facecolor("white")

            # highlight new group
            if found_id is not None:
                for p in self.id_to_patches[found_id]:
                    p.set_facecolor("orange")

            self.highlighted_id = found_id

            # update label
            if found_id is None:
                self.id_label.set_text("ID: None")
            else:
                self.id_label.set_text(f"ID: {found_id}")

            self.fig.canvas.draw_idle()

    def on_scroll(self, event):

        scale = 0.9 if event.button == "up" else 1.1

        x0, x1 = self.ax.get_xlim()
        y0, y1 = self.ax.get_ylim()

        cx = event.xdata
        cy = event.ydata

        new_w = (x1 - x0) * scale
        new_h = (y1 - y0) * scale

        self.ax.set_xlim(cx - new_w / 2, cx + new_w / 2)
        self.ax.set_ylim(cy - new_h / 2, cy + new_h / 2)

        self.fig.canvas.draw_idle()


def main():

    if len(sys.argv) < 2:
        print("Usage: python rectangles.py rectangles.txt")
        return

    rectangles = load_rectangles(sys.argv[1])
    viewer = RectangleViewer(rectangles)

    plt.show()


if __name__ == "__main__":
    main()