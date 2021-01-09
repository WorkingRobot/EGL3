#include "ChatBubble.h"

#include "../utils/Assert.h"
#include "../utils/Humanize.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace EGL3::Widgets {
    ChatBubble::ChatBubble(Storage::Models::ChatMessage& Message) :
        Message(Message)
    {
        Content.set_text(Message.Content);

        Construct();
    }

    ChatBubble::operator Gtk::Widget& () {
        return BaseContainer;
    }

    void ChatBubble::Construct()
    {
        Content.set_line_wrap(true);
        Content.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        //Content.set_max_width_chars(1);
        //Content.set_hexpand(false);
        /*
        Content.signal_size_allocate().connect([this](Gtk::Allocation& Alloc) {
            printf("Alloc\n");
            Alloc.set_width(BaseContainer.get_allocation().get_width() * .75f);
        });*/

        BubbleContainer.set_has_tooltip(true);
        BubbleContainer.signal_query_tooltip().connect([this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
            tooltip->set_text(Utils::Humanize(Message.Time));
            return true;
        });
        BubbleContainer.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) {
            Gdk::RGBA Color;
            if (EGL3_CONDITIONAL_LOG(BaseContainer.get_style_context()->lookup_color(Message.Recieved ? "fg_color" : "selected_bg_color", Color), LogLevel::Warning, "Failed to get chat background color")) {
                // It's a foreground color, don't make it so apparent
                if (Message.Recieved) {
                    Color.set_alpha(Color.get_alpha() * .1);
                }
            }
            else {
                Color = BaseContainer.get_style_context()->get_color(Gtk::STATE_FLAG_NORMAL);
                Color.set_alpha(Color.get_alpha() * .1);
            }

            Ctx->save();

            Ctx->set_source_rgba(Color.get_red(), Color.get_green(), Color.get_blue(), Color.get_alpha());
            DrawBubble(Ctx, BubbleContainer.get_allocation(), Message.Recieved);
            Ctx->fill();

            Ctx->restore();
            return false;
        }, false);

        auto HAlign = Message.Recieved ? Gtk::ALIGN_START : Gtk::ALIGN_END;

        Content.set_halign(Gtk::ALIGN_CENTER);
        Content.set_xalign(0);
        Content.set_yalign(0);
        Content.set_margin_start(5);
        Content.set_margin_end(12);
        Content.set_margin_top(7);
        Content.set_margin_bottom(7);

        BubbleContainer.set_halign(HAlign);

        BubbleContainer.pack_start(Content, true, true, 0);

        BaseContainer.pack_start(BubbleContainer, false, false, 0);

        BaseContainer.show_all();
    }

    void ChatBubble::DrawBubble(const Cairo::RefPtr<Cairo::Context>& Ctx, Gtk::Allocation Alloc, bool Recieved) {
        double x = 0;
        double y = 0;
        double width = Alloc.get_width();
        double height = Alloc.get_height();
        double radius = std::min(std::min(width, height), 40.) / 2;

        double degrees = M_PI / 180.0;

        Ctx->arc(x + width - radius, y + radius, radius, -.5 * M_PI, 0);
        if (Recieved) {
            Ctx->arc(x + width - radius, y + height - radius, radius, 0, .5 * M_PI);
            Ctx->line_to(x, y + height);
        }
        else {
            Ctx->line_to(x + width, y + height);
            Ctx->arc(x + radius, y + height - radius, radius, .5 * M_PI, M_PI);
        }
        Ctx->arc(x + radius, y + radius, radius, M_PI, 1.5 * M_PI);
    }
}
