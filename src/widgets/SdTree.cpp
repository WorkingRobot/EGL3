#include "SdTree.h"

#include "../utils/Platform.h"

namespace EGL3::Widgets {
    SdTree::SdTree(Gtk::TreeView& TreeView) :
        TreeView(TreeView)
    {
        DefaultEvaluator.AddFunction("RandomAccessMemoryGB", [](const std::string& Input) -> int64_t {
            return Utils::Platform::GetMemoryConstants().TotalPhysicalGB;
        });
        DefaultEvaluator.AddFunction("HasAccess", [](const std::string& Input) -> bool {
            // TODO: implement library check/client or something
            // By default, we assume the user doesn't have access to anything
            return false;
        });

        TreeStore = Gtk::TreeStore::create(Columns);
        TreeFilter = Gtk::TreeModelFilter::create(TreeStore);

        TreeFilter->set_visible_func([this](const Gtk::TreeModel::const_iterator& Itr) {
            auto& Row = *Itr;
            auto& Opt = *Row[Columns.Data];
            if (&Opt == nullptr) {
                return true;
            }
            return !Opt.Invisible;
        });

        TreeView.set_model(TreeFilter);
        TreeView.set_headers_visible(false);
        TreeView.signal_query_tooltip().connect([this](int X, int Y, bool KeyboardTooltip, const Glib::RefPtr<Gtk::Tooltip>& Tooltip) {
            Gtk::TreeModel::Path Path;
            if (KeyboardTooltip) {
                Gtk::TreeViewColumn* Column;
                this->TreeView.get_cursor(Path, Column);
                if (Path.empty()) {
                    return false;
                }
            }
            else {
                int BinX, BinY;
                this->TreeView.convert_widget_to_bin_window_coords(X, Y, BinX, BinY);
                if (!this->TreeView.get_path_at_pos(BinX, BinY, Path) || Path.empty()) {
                    return false;
                }
            }

            auto Itr = TreeFilter->get_iter(Path);
            if (!Itr) {
                return false;
            }

            auto& Row = *Itr;
            auto& Opt = *Row[Columns.Data];
            auto& Desc = Opt.Description.Get();
            if (Desc.empty()) {
                return false;
            }
            Tooltip->set_text(Desc);

            this->TreeView.set_tooltip_row(Tooltip, Path);
            return true;
        });
        TreeView.set_has_tooltip(true);

        TreeView.append_column("Name", Columns.Name);

        {
            RequiredRenderer.property_markup().set_value("<i>Required</i>");
            int ColCount = TreeView.append_column("Required", RequiredRenderer);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                Column->add_attribute(RequiredRenderer.property_visible(), Columns.Required);
            }
        }

        {
            int ColCount = TreeView.append_column("Selected", Columns.Selected);
            auto Column = TreeView.get_column(ColCount - 1);
            if (Column) {
                auto Renderer = (Gtk::CellRendererToggle*)Column->get_first_cell();
                Renderer->set_activatable(true);
                Column->add_attribute(Renderer->property_visible(), Columns.Selectable);
            }
        }

        TreeView.set_activate_on_single_click();
        TreeView.get_selection()->set_select_function([this](const Glib::RefPtr<Gtk::TreeModel>& Model, const Gtk::TreeModel::Path& Path, bool CurrentlySelected) {
            auto Itr = TreeFilter->get_iter(Path);
            if (Itr) {
                auto& Row = *Itr;
                if (Row[Columns.Selectable] && !Row[Columns.Required]) {
                    auto& Opt = *Row[Columns.Data];
                    // Toggle the selection when the user tries to click on the row
                    Row[Columns.Selected] = !Row[Columns.Selected];
                }
            }
            return false;
        });

        TreeView.show_all();
    }

    void SdTree::Initialize(const std::vector<Web::Epic::Content::SdMeta::Data>* Options)
    {
        TreeStore->clear();
        if (Options) {
            OptionsData = *Options;
        }
        else {
            OptionsData.clear();
        }

        // We only need to recurse once, no sdmetas have nested children so far
        for (auto& Opt : OptionsData) {
            auto Itr = TreeStore->append();
            auto& Row = *Itr;

            for (auto& ChildOpt : Opt.Children) {
                auto ChildItr = TreeStore->append(Row.children());
                auto& ChildRow = *ChildItr;
                ChildRow[Columns.Data] = &ChildOpt;
                if (!ChildOpt.Invisible) {
                    ChildRow[Columns.Name] = ChildOpt.Title.Get();
                    ChildRow[Columns.Required] = ChildOpt.IsRequired;
                    ChildRow[Columns.Selected] = false;
                    ChildRow[Columns.Selectable] = !ChildOpt.InstallTags.empty();
                }
            }

            Row[Columns.Data] = &Opt;
            if (!Opt.Invisible) {
                Row[Columns.Name] = Opt.Title.Get();
                Row[Columns.Required] = Opt.IsRequired;
                Row[Columns.Selected] = false;
                Row[Columns.Selectable] = !Opt.InstallTags.empty();
                if (Opt.IsDefaultExpanded) {
                    TreeView.expand_row(TreeFilter->convert_child_path_to_path(TreeStore->get_path(Itr)), false);
                }
            }
        }
    }

    void SdTree::SetOptions(const std::vector<std::string>& UniqueIds)
    {
        SelectedIds = UniqueIds;
        TreeFilter->foreach_iter([this, &UniqueIds](const Gtk::TreeModel::iterator& Itr) {
            auto& Row = *Itr;
            auto& Opt = *Row[Columns.Data];
            Row[Columns.Selected] = Opt.IsRequired ||
                                    std::find(UniqueIds.begin(), UniqueIds.end(), Opt.Id) != UniqueIds.end() ||
                                    std::find(UniqueIds.begin(), UniqueIds.end(), Opt.UpgradePathLogic) != UniqueIds.end();

            return false;
        });
    }

    template<bool OnlyDefault>
    void AppendSelectedIds(const std::vector<Web::Epic::Content::SdMeta::Data>& Data, const Utils::StringEx::ExpressionEvaluator& DefaultEvaluator, const std::string& ConfigHandler, std::vector<std::string>& SelectedIds)
    {
        for (auto& Opt : Data) {
            if (!Opt.InstallTags.empty()) {
                if constexpr (OnlyDefault) {
                    if ((Opt.IsRequired) ||
                        (Opt.IsDefaultSelected && DefaultEvaluator.Evaluate(Opt.DefaultSelectedExpression, "")) ||
                        (ConfigHandler == "DefaultLanguage" && Opt.ConfigValue == Utils::Config::GetLanguage())) {
                        SelectedIds.emplace_back(Opt.Id);
                    }
                }
                else {
                    SelectedIds.emplace_back(Opt.Id);
                }
            }

            AppendSelectedIds<OnlyDefault>(Opt.Children, DefaultEvaluator, Opt.ConfigHandler, SelectedIds);
        }
    }

    void SdTree::SetDefaultOptions()
    {
        SelectedIds.clear();
        AppendSelectedIds<true>(OptionsData, DefaultEvaluator, "", SelectedIds);
        SetOptions(SelectedIds);
    }

    void SdTree::SetAllOptions()
    {
        SelectedIds.clear();
        AppendSelectedIds<false>(OptionsData, DefaultEvaluator, "", SelectedIds);
        SetOptions(SelectedIds);
    }

    void SdTree::GetOptions(std::vector<std::string>& UniqueIds, std::vector<std::string>& InstallTags)
    {
        UniqueIds.clear();
        InstallTags.clear();
        // Find all required and selected ids
        TreeFilter->foreach_iter([this, &UniqueIds, &InstallTags](const Gtk::TreeModel::iterator& Itr) {
            auto& Row = *Itr;
            auto& Opt = *Row[Columns.Data];
            if (Opt.IsRequired || Row[Columns.Selected]) {
                UniqueIds.emplace_back(Opt.Id);
                for (auto& Tag : Opt.InstallTags) {
                    InstallTags.emplace_back(Tag);
                }
            }

            return false;
        });

        Utils::StringEx::ExpressionEvaluator InvisibleEvaluator;
        InvisibleEvaluator.AddFunction("IsComponentSelected", [&UniqueIds](const std::string& Input) -> bool {
            return std::find(UniqueIds.begin(), UniqueIds.end(), Input) != UniqueIds.end();
        });

        // At this point, the invisible selection expression now should have enough info to be correctly evaluated
        TreeStore->foreach_iter([this, &InvisibleEvaluator, &UniqueIds, &InstallTags](const Gtk::TreeModel::iterator& Itr) {
            auto& Row = *Itr;
            auto& Opt = *Row[Columns.Data];
            if (Opt.Invisible && !Opt.IsRequired) {
                if (InvisibleEvaluator.Evaluate(Opt.InvisibleSelectedExpression, "")) {
                    UniqueIds.emplace_back(Opt.Id);
                    for (auto& Tag : Opt.InstallTags) {
                        InstallTags.emplace_back(Tag);
                    }
                }
            }

            return false;
        });
    }
}
