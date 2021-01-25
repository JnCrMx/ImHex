#include <hex/plugin.hpp>

#include <iostream>
#include <proc/readproc.h>

#include "providers/procmem_provider.hpp"
extern "C" {
#include "external/pmparser.h"
}

namespace hex::plugin::procmem {
    void drawProcessList() {
        ImGui::BeginTable("##processlist", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg);

        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("name");
        ImGui::TableSetupColumn("user");
        ImGui::TableHeadersRow();

        PROCTAB* proc = openproc(PROC_FILLSTAT | PROC_FILLUSR);
        proc_t proc_info;
        memset(&proc_info, 0, sizeof(proc_info));

        u32 rowCount = 0;
        int selectedPid = 0;
        while (readproc(proc, &proc_info) != NULL) {
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

            ImGui::TableNextColumn();

            std::string pid = std::to_string(proc_info.tid);
            if (ImGui::Button(pid.c_str())) {
                selectedPid = proc_info.tid;
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", proc_info.cmd);

            ImGui::TableNextColumn();
            ImGui::Text("%s", proc_info.euser);

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ((rowCount % 2) == 0) ? 0xFF101010 : 0xFF303030);
            rowCount++;
        }
        closeproc(proc);

        ImGui::EndTable();

        ImGui::NewLine();

        if(selectedPid) {
            SharedData::bookmarkEntries.clear();
            u64 testAddress, lastAddress;

            procmaps_iterator* maps = pmparser_parse(selectedPid);
            if (maps == NULL) {
                std::cerr << "Cannot open memory maps for process " << selectedPid << std::endl;
                return;
            }
            procmaps_struct* maps_entry=NULL;
            while ((maps_entry = pmparser_next(maps)) != NULL) {
                std::string name(maps_entry->pathname);
                std::string comment(maps_entry->perm);

                ImHexApi::Bookmarks::add((u64)maps_entry->addr_start, maps_entry->length, name, comment, 0);
                if(name=="[stack]") { // the stack should be readable and writable
                    testAddress = (u64)maps_entry->addr_start;
                }
                if(name!="[vsyscall]") { // vsyscall is so far that it would totally smash the size
                    lastAddress = (u64)maps_entry->addr_end;
                }
            }
            pmparser_free(maps);

            auto& provider = SharedData::currentProvider;

            if (provider != nullptr)
                delete provider;
            provider = new hex::ProcMemProvider(selectedPid, lastAddress, testAddress);
            if (provider->isAvailable()) {
                View::postEvent(Events::FileLoaded);
                View::postEvent(Events::DataChanged);
                View::postEvent(Events::PatternChanged);
            } else {
                SharedData::bookmarkEntries.clear();
            }
        }
    }
}

IMHEX_PLUGIN_SETUP {
	using namespace hex::plugin::procmem;

    ContentRegistry::Tools::add("Open process memory", drawProcessList);
}


