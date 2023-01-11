module("luci.controller.admin.hlk", package.seeall)  --notice that new_tab is the name of the file new_tab.lua
 function index()
	local page
     entry({"admin", "hlk"}, firstchild(), "HLK", 60).dependent=true  --this adds the top level tab and defaults to the first sub-tab (tab_from_cbi), also it is set to position 30
--     entry({"admin", "hlk", "tab_from_cbi"}, cbi("hlk/cbi_tab"), "CBI Tab", 1)  --this adds the first sub-tab that is located in <luci-path>/luci-myapplication/model/cbi/myapp-mymodule and the file is called cbi_tab.lua, also set to first position
--     entry({"admin", "hlk", "tab_from_view"}, template("hlk/view_tab"), "View Tab", 2)  --this adds the second sub-tab that is located in <luci-path>/luci-myapplication/view/myapp-mymodule and the file is called view_tab.htm, also set to the second position

--	 page = node("admin", "hlk", "5G")
--	 page.target = view("hlk/5G")
--	 page.title  = _("5G")
--	 page.order  = 30
end

