/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PVZ_CHEAT_H
#define PVZ_CHEAT_H

/**
 * @file 修改器菜单相关
 *
 * 区域命名空间的命名由 'language'(必需), 'script', 'country' 等组成:
 * 'language' 为 两个字母组成的 ISO 639-1 语言代码
 * 'script' 为 四个字母组成的 ISO 15924 文字代码
 * 'country' 为 两个字母组成的 ISO 3166-1-alpha-2 区域码
 */

#include <algorithm>
#include <array>

namespace cheat {

template <std::size_t N>
using StringArray = std::array<const char *const, N>;

using SettingsList = StringArray<4>;
using FeatureList = StringArray<123>;

template <std::size_t N>
consteval bool CheckList(const StringArray<N> &list) {
    return std::ranges::none_of(list, [](const char *item) { return item == nullptr || *item == '\0'; });
}

namespace lang {

    namespace en_US {
        inline constexpr SettingsList settingsList = {
            "Category_Cheat Menu Settings",
            "-1_Toggle_Save Settings on Exit", //-1 is checked on Preferences.java
            "Category_More Options",
            "-6_Button_<font color='yellow'>Back to Cheat Menu</font>",
        };

        inline constexpr FeatureList featureList = {
            "Collapse_Common",
            "1_CollapseAdd_Toggle_Unlimited Sun",
            "42_CollapseAdd_Toggle_Placed Anywhere",
            "2_CollapseAdd_Toggle_Seeds No CD",
            "3_CollapseAdd_Toggle_Reload Instantly",
            "4_CollapseAdd_Toggle_Mushroom Awake",
            "5_CollapseAdd_Toggle_Advanced Pause",
            "8_CollapseAdd_Spinner_<font color='green'>Set Game Speed：_Off,1.2x,1.5x,2x,2.5x,3x,5x,10x",
            "82_CollapseAdd_OnceCheckBox_Level Complete",


            "Collapse_Debug",
            "CollapseAdd_RichTextView_<font color='green'>Show Plant Health:",
            "21_CollapseAdd_Toggle_All Plants",
            "22_CollapseAdd_Toggle_Cob Cannon and Tough Plant",
            "CollapseAdd_RichTextView_<font color='green'>Show Zombie Health:",
            "23_CollapseAdd_Toggle_Health of Body",
            "24_CollapseAdd_Toggle_Heath of Helm and Shield",
            "25_CollapseAdd_Toggle_Gargantuar",
            "CollapseAdd_RichTextView_<font color='green'>Show Game Message:",
            "26_CollapseAdd_Toggle_Zombie Spawn",
            "27_CollapseAdd_Toggle_Collision",


            "Collapse_Entertainment",
            "6_CollapseAdd_Toggle_Clear Fog",
            "43_CollapseAdd_Toggle_Transparent Vase",
            "45_CollapseAdd_Toggle_Zombie Cannot Enter House",
            "44_CollapseAdd_Toggle_Wall-nut Cause Zombie Bloat Certainly",
            "7_CollapseAdd_Toggle_Not Drop Loot",
            "48_CollapseAdd_Spinner_<font color='green'>Control Dancing Zombies：_Off,Keep Moving Forward,Keep Dancing in Place,Keep Trying Summon",
            "46_CollapseAdd_Spinner_<font color='green'>Further Back Baseline：_Off,10,20,30,40,50,60,70,80",
            "47_CollapseAdd_Spinner_<font color='green'>Set Zombie Size：_Off,0.2x,0.4x,0.6x,0.8x,1x,1.2x,1.4x,1.6x,1.8x,2x",
            "CollapseAdd_RichTextView_<font color='yellow'># Blood volume is squarely related to size!",
            "41_CollapseAdd_OnceCheckBox_Cheat Code Dialog",


            "Collapse_Level Setting",
            "87_CollapseAdd_Toggle_Spawning Pause",
            "88_CollapseAdd_Toggle_No Lawn Mower",
            "83_CollapseAdd_Toggle_No Seeds Picked by Crazy Dave",
            "84_CollapseAdd_Toggle_Last Stand Endless",
            "89_CollapseAdd_Toggle_Disable Deleting Save Data",
            "90_CollapseAdd_Toggle_Disable Saving Save Data",
            "81_CollapseAdd_OnceCheckBox_Cheat Dialog",
            "102_CollapseAdd_Spinner_<font color='green'>Set Game Scene：_Off,Day,Night,Pool,Fog,Roof,Night Roof,Zen Garden,Mushroom Garden,Aquarium Garden",
            "CollapseAdd_RichTextView_<font color='green'>Set Rounds of Survival Endless:",
            "85_CollapseAdd_InputValue_The Rounds Your Want",
            "86_CollapseAdd_OnceCheckBox_Set Rounds",
            "CollapseAdd_RichTextView_<font color='yellow'># Set Rounds ONLY after Completing Seed Choosing!",


            "Collapse_Seed Slot Setting",
            "141_CollapseAdd_Spinner_<font color='green'>The Side of Slots：_Left Slots,Right Slots",
            "142_CollapseAdd_Spinner_<font color='green'>Target Seed Slot：_1st,2nd,3rd,4th,5th,6th,7th,8th,9th,10th",
            //            "143_CollapseAdd_Spinner_<font
            //            color='green'>全部卡片类型：_关闭,豌豆射手,向日葵,樱桃炸弹,坚果,土豆地雷,寒冰射手,大嘴花,双重射手,小喷菇,阳光菇,大喷菇,咬咬碑,迷惑菇,胆小菇,冰川菇,末日菇,莲叶,窝瓜,三重射手,缠绕水草,火爆辣椒,地刺,火炬树桩,高坚果,水兵菇,路灯花,仙人掌,三叶草,双向射手,星星果,南瓜头,磁力菇,卷心菜投手,花盆,玉米投手,咖啡豆,大蒜,叶子保护伞,金盏花,西瓜投手,机枪射手,双胞向日葵,多嘴小蘑菇,猫尾草,冰西瓜,吸金磁,钢地刺,玉米加农炮,变身茄子,无(可选植物种类总数),爆炸坚果,巨型坚果,幼苗,反向双重射手,无(植物种类总数),刷新阵型(僵尸迷阵),消除炸弹坑(僵尸迷阵),阳光(拉霸),钻石(拉霸),潜水僵尸(僵尸水族馆),奖杯(僵尸水族馆),墓碑,普通僵尸,垃圾桶僵尸,路障僵尸,撑杆僵尸,铁桶僵尸,旗帜僵尸,报纸僵尸,铁网门僵尸,橄榄球僵尸,舞者僵尸,雪橇车僵尸,玩偶匣僵尸,矿工僵尸,蹦蹦僵尸,蹦极僵尸,梯子僵尸,投石车僵尸,巨人僵尸,无,鸭子救生圈僵尸,潜水僵尸,海豚骑士僵尸,小鬼僵尸,气球僵尸",
            "143_CollapseAdd_Spinner_<font color='green'>Seed Type："
            "_Not Selected,Peashooter,Sunflower,Cherry Bomb,Wall-nut,Potato Mine,Snow Pea,Chomper,Repeater,"
            "Puff-shroom,Sun-shroom,Fume-shroom,Grave Buster,Hypno-shroom,Scaredy-shroom,Ice-shroom,Doom-shroom,"
            "Lily Pad,Squash,Threepeater,Tangle Kelp,Jalapeno,Spikeweed,Torchwood,Tall-nut,"
            "Sea-shroom,Plantern,Cactus,Blover,Split Pea,Starfruit,Pumpkin,Magnet-shroom,"
            "Cabbage-pult,Flower Pot,Kernel-pult,Coffee Bean,Garlic,Umbrella Leaf,Marigold,Melon-pult,"
            "Gatlingp Pea,Twin Sunflower,Gloom-shroom,Cattail,Winter Melon,Gold Magnet,Spikerock,Cob Cannon,"
            "Explode-o-nut,Giant Wall-nut,Sprout,Leftpeater,"
            "Grave Stone,Zombie,Trash Bin Zombie,Conehead Zombie,Pole Vaulting Zombie,Buckethead Zombie,"
            "Flag Zombie,Newspaper Zombie,Screen Door Zombie,Football Zombie,Dancing Zombie,Zomboni,"
            "Jack-in-the-box Zombie,Digger Zombie,Pogo Zombie,Bungee Zombie,Ladder Zombie,Catapult Zombie,"
            "Gargantuar,Zombie Yeti,Zombie Bobsled Team,Snorkel Zombie,Dolphin Rider Zombie,Imp,Ballon Zombie",
            "144_CollapseAdd_CheckBox_Imitater Plant Seed",
            "145_CollapseAdd_OnceCheckBox_Replace Seed",


            "Collapse_Projectile Setting",
            "62_CollapseAdd_Toggle_Deal Damage at Every Frame",
            "61_CollapseAdd_Toggle_Torchwood Ignored by Snow Pea",
            "63_CollapseAdd_Spinner_<font color='green'>Modify Projectile Type：_Off,Pea,SnowPea,Cabbage,Melon,Puff,WinterMelon,Fireball,Star,Spike,Basketball,Kernel,CobBig,Butter,Zombie Pea",
            "64_CollapseAdd_Toggle_Random Projectile Type",
            "65_CollapseAdd_Toggle_Only Change The Type of Pea",
            "66_CollapseAdd_Toggle_Pea Change Type after Hitting Torchwood",
            "67_CollapseAdd_CheckBox_Random Type Exclude BigCob",
            "68_CollapseAdd_CheckBox_Random Type Exclude Star",


            "Collapse_Spawn Setting",
            "CollapseAdd_RichTextView_<font color='green'>Check Zombie Type:",
            "200_CollapseAdd_CheckBox_Zombie(Appear in Natural Spawn Mode Certainly)",
            "202_CollapseAdd_CheckBox_Conehead Zombie",
            "203_CollapseAdd_CheckBox_Pole Vaulting Zombie",
            "204_CollapseAdd_CheckBox_Buckethead Zombie",
            "205_CollapseAdd_CheckBox_Newspaper Zombie",
            "206_CollapseAdd_CheckBox_Screen Door Zombie",
            "207_CollapseAdd_CheckBox_Football Zombie",
            "208_CollapseAdd_CheckBox_Dancing Zombie",
            "211_CollapseAdd_CheckBox_Snorkel Zombie",
            "212_CollapseAdd_CheckBox_Zomboni",
            "214_CollapseAdd_CheckBox_Dolphin Rider Zombie",
            "215_CollapseAdd_CheckBox_Jack-in-the-box Zombie",
            "216_CollapseAdd_CheckBox_Ballon Zombie",
            "217_CollapseAdd_CheckBox_Digger Zombie",
            "218_CollapseAdd_CheckBox_Pogo Zombie",
            "219_CollapseAdd_CheckBox_Zombie Yeti",
            "220_CollapseAdd_CheckBox_Bungee Zombie",
            "221_CollapseAdd_CheckBox_Ladder Zombie",
            "222_CollapseAdd_CheckBox_Catapult Zombie",
            "223_CollapseAdd_CheckBox_Gargantuar",
            "233_CollapseAdd_CheckBox_Redeye Gargantuar",
            "226_CollapseAdd_CheckBox_Trash Bin Zombie",
            "227_CollapseAdd_CheckBox_Pea Head Zombie",
            "228_CollapseAdd_CheckBox_Wall-nut Head Zombie",
            "229_CollapseAdd_CheckBox_Jalapeno Head Zombie",
            "230_CollapseAdd_CheckBox_Gatling Zombie",
            "231_CollapseAdd_CheckBox_Squash Head Zombie",
            "232_CollapseAdd_CheckBox_Tall-nut Head Zombie",
            "234_CollapseAdd_CheckBox_All-Star Zombie",
            "235_CollapseAdd_CheckBox_Super-Fan Imp Zombie",
            "241_CollapseAdd_Spinner_<font color='green'>Choose Spawn Mode：_Off,Natural (Zombies in Wave is Automatically Picked by The Game),Extreme (Populate Zombies in Wave Evenly)",
            "242_CollapseAdd_OnceCheckBox_Set Zombie Spawn",


            "Collapse_Battlefield Layout",
            "101_CollapseAdd_Toggle_Add Ladder when Planting Pumpkin",
            "105_CollapseAdd_Spinner_<font color='yellow'>Target Column："
            "_1st Column,2nd Column,3rd Column,4th Column,5th Column,6th Column,7th Column,8th Column,9th Column,All Columns,Zombie Spawning Location",
            "106_CollapseAdd_Spinner_<font color='yellow'>Target Row：_1st Row,2nd Row,3rd Row,4th Row,5th Row,6th Row,All Rows",
            "103_CollapseAdd_Spinner_<font color='green'>Plant Type："
            "_Not Selected,Peashooter,Sunflower,Cherry Bomb,Wall-nut,Potato Mine,Snow Pea,Chomper,Repeater,"
            "Puff-shroom,Sun-shroom,Fume-shroom,Grave Buster,Hypno-shroom,Scaredy-shroom,Ice-shroom,Doom-shroom,"
            "Lily Pad,Squash,Threepeater,Tangle Kelp,Jalapeno,Spikeweed,Torchwood,Tall-nut,"
            "Sea-shroom,Plantern,Cactus,Blover,Split Pea,Starfruit,Pumpkin,Magnet-shroom,"
            "Cabbage-pult,Flower Pot,Kernel-pult,Coffee Bean,Garlic,Umbrella Leaf,Marigold,Melon-pult,"
            "Gatlingp Pea,Twin Sunflower,Gloom-shroom,Cattail,Winter Melon,Gold Magnet,Spikerock,Cob Cannon,"
            "Explode-o-nut,Giant Wall-nut,Sprout,Leftpeater",
            "107_CollapseAdd_CheckBox_Imitater Plant",
            "108_CollapseAdd_OnceCheckBox_Add Plant",
            "104_CollapseAdd_Spinner_<font color='green'>Zombie Type："
            "_Not Selected,Zombie,Flag Zombie,Conehead Zombie,Pole Vaulting Zombie,Buckethead Zombie,"
            "Newspaper Zombie,Screen Door Zombie,Football Zombie,Dancing Zombie,Backup Dancer,"
            "Ducky Tube Zombie,Snorkel Zombie,Zomboni,Zombie Bobsled Team,Dolphin Rider Zombie,"
            "Jack-in-the-box Zombie,Ballon Zombie,Digger Zombie,Pogo Zombie,Zombie Yeti,"
            "Bungee Zombie,Ladder Zombie,Catapult Zombie,Gargantuar,Imp,Dr. Zomboss,"
            "Trash Bin Zombie,Pea Head Zombie,Wall-nut Head Zombie,Jalapeno Head Zombie,Gatling Zombie,Squash Head Zombie,Tall-nut Head Zombie,Redeye Gargantuar,"
            "All-Star Zombie, Super-Fan Imp Zombie",
            "109_CollapseAdd_OnceCheckBox_Add Zombie",
            "CollapseAdd_RichTextView_<font color='green'>Other Object:",
            "114_CollapseAdd_OnceCheckBox_Add Grave Stone",
            "110_CollapseAdd_OnceCheckBox_Add Ladder",
            "111_CollapseAdd_OnceCheckBox_Reset All Mowers",
            "CollapseAdd_RichTextView_<font color='yellow'>Remove:",
            "112_CollapseAdd_OnceCheckBox_Remove All Plnats",
            "115_CollapseAdd_OnceCheckBox_Remove All Zombies",
            "116_CollapseAdd_OnceCheckBox_Remove All Grave Stones",
            "113_CollapseAdd_OnceCheckBox_Remove All Mowers",
            "CollapseAdd_RichTextView_<font color='yellow'>Set State:",
            "9_CollapseAdd_OnceCheckBox_Hypnotize All Zombies",
            "10_CollapseAdd_OnceCheckBox_Ice All Zombies",
            "11_CollapseAdd_OnceCheckBox_Start All Mowers",


            "Collapse_Quick Embattle",
            "121_CollapseAdd_Spinner_<font color='green'>Choose Formation for Pool：_Not Selected,"
            "[0]Radio Clock Cobless,[1]Minimalist Cobless,[2]Pseudo-Unharmed Cobless,[3]Automatically Subdue Jack-in-the-box Cobless,[4]Fiery Cobless,"
            "[5]Split Fiery Cobless,[6]Rear Cobless,[7]Super Forward Cobless,[8]Prince Cobless,[9]Mechanical Clock Cobless,"
            "[10]Sideless Cobless,[11]Quartz Clock Cobless,[12]Sunflowerless Cobless,[13]Square Gloomless Cobless,[14]56 Row Accelerated Gloomless Cobless,"
            "[15]1 Cob Suppression,[16]Little ② Cob,[17]Fiery 2 Cob,[18]Nuclear 2 Cob,[19]Split 2 Cob,"
            "[20]Square 2 Cob,[21]Classic 2 Cob,[22]Speedrun 3 Cob,[23]Tai Chi 4 Cob,[24]Metallic 4 Cob,"
            "[25]Square 4 Cob,[26]Verdant 4 Cob,[27]Waterless 4 Cob,[28]Diamond 4 Cob,[29]Sideless 4 Cob,"
            "[30]Nuclear Back 4 Cob,[31]Classic 4 Cob,[32]Fiery 4 Cob,[33]Back 4 Cob,[34]Traditional 4 Cob,"
            "[35]Half 5 Cob,[36]Scattershot 5 Cob,[37]Heart 5 Cob,[38]Landless 6 Cob,[39]Waterless 6 Cob,"
            "[40]Verdant 6 Cob,[41]Zen Room Deep in Flowers and Plants,[42]Sideless 6 Cob,[43]Kernel 6 Cob,[44]Air Bomb 6 Cob,"
            "[45]Super Rear 6 Cob,[46]Square 6 Cob,[47]Butterfly Rhyme,[48]A Spoonful of Sweet Soup Balls,[49]Separated 7 Cob,"
            "[50]Jade Rabbit,[51]Pumpkinless 8 Cob,[52]Christmas Tree 8 Cob,[53]Centered Christmas Tree 8 Cob,[54]Rectangular 8 Cob,"
            "[55]Sideless 8 Cob,[56]Yin and Yang 8 Cob,[57]Duckweed 8 Cob,[58]Rear 8 Cob,[59]To Feed a Dolphin,"
            "[60]Kernel 8 Cob,[61]Classic 8 Cob,[62]Sea of Flowers 8 Cob,[63]C2 8 Cob,[64]Separation 8 Cob,"
            "[65]Full-Symmetrical 8 Cob,[66]3C 8 Cob,[67]Lampstand 8 Cob,[68]⑨ Cob,[69]Square 9 Cob,"
            "[70]C6i 9 Cob,[71]Heart 9 Cob,[72]Alternate 9 Cob,[73]② Cob,[74]Hexagram 10 Cob,"
            "[75]Hexagon 10 Cob,[76]Square 10 Cob,[77]Diamond 10 Cob,[78]Simplified 10 Cob,[79]Rear 10 Cob,"
            "[80]Classic 10 Cob,[81]To Imprison 6 Zombies,[82]Diagonal 10 Cob,[83]Rubik's Cube 10 Cob,[84]Dave's Hamburger,"
            "[85]Cocktail,[86]A Spoonful of Sweet Soup Balls 12 Cob,[87]Porcelain Vase 12 Cob,[88]Half 12 Cob,[89]Simplified 12 Cob,"
            "[90]Classic 12 Cob,[91]Fiery 12 Cob,[92]Ice Rain 12 Cob (remodelled),[93]Ice Rain 12 Cob (remodelled x2),[94]Single Purple Card 12 Cob,"
            "[95]Sideless Column 12 Cob,[96]Sideless 12 Cob,[97]Waterless 12 Cob,[98]Pure White Card Impending 12 Cob,[99]Backyard 12 Cob,"
            "[100]Kernel 8 Cob,[101]Two Row Accelerated 12 Cob,[102]Front 12 Cob,[103]Ladder & Gloom-shroom 12 Cob,[104]Jun Hai 12 Cob,"
            "[105]A Poem for The Harp,[106]Plum Blossom 13 Cob,[107]Last Order,[108]Ice Lantern 14 Cob,[109]Tai Chi 14 Cob,"
            "[110]Real ④ Cob,[111]God Stick 14 Cob,[112]Sideless 14 Cob,[113]Timeslip 14 Cob,[114]Diamond 15 Cob,"
            "[115]Sideless 15 Cob,[116]Real ② Cob,[117]Icebox 16 Cob,[118]Cob Ring 12 Sunflower,[119]Single Ice 16 Cob,"
            "[120]Symmetrical 16 Cob,[121]Sideless 16 Cob,[122]Streaking 16 Cob,[123]Double Ice 16 Cob,[124]Super Forward 16 Cob,"
            "[125]Fiery 16 Cob,[126]Classic 16 Cob,[127]Zigzag Line 16 Cob,[128]Lung 18 Cob (extreme),[129]Pure 18 Cob,"
            "[130]Real 18 Cob,[131]Ice Soul 18 Cob,[132]Tail Bomb 18 Cob,[133]Classic 18 Cob,[134]Pure 20 Cob,"
            "[135]Air Bomb 20 Cob,[136]Rake 20 Cob,[137]New 20 Cob,[138]Winter-melonless 20 Cob,[139]The Road of Despair,"
            "[140]21 Cob,[141]New 22 Cob,[142]22 Cob,[143]Winter-melonless 22 Cob,[144]Front 22 Cob,"
            "[145]24 Cob,[146]Cannon Fodder 24 Cob,[147]Scaredy-shroom Protecter (extreme)",
            "122_CollapseAdd_OnceCheckBox_Deploy Formation",
            "CollapseAdd_RichTextView_<font color='green'>Formation Export/Import:",
            "123_CollapseAdd_FormationCopy_Copy Formation Code",
            "124_CollapseAdd_InputText_Paste Formation Code",
            "125_CollapseAdd_OnceCheckBox_Deploy Formation",
            "CollapseAdd_RichTextView_<font color='yellow'># You can deploy formation in pause",
        };

        static_assert(CheckList(settingsList));
        static_assert(CheckList(featureList));
    } // namespace en_US

    namespace vi {
        inline constexpr SettingsList settingsList = {
            "Category_Cài đặt Menu Cheat",
            "-1_Toggle_Lưu cài đặt khi thoát", //-1 is checked on Preferences.java
            "Category_Tùy chọn khác",
            "-6_Button_<font color='yellow'>Quay lại Menu Cheat</font>",
        };

        inline constexpr FeatureList featureList = {
            "Collapse_Chức năng thường dùng",
            "1_CollapseAdd_Toggle_Mặt trời vô hạn",
            "42_CollapseAdd_Toggle_Trồng cây mọi nơi",
            "2_CollapseAdd_Toggle_Thẻ không hồi chiêu",
            "3_CollapseAdd_Toggle_Nạp đạn tức thì",
            "4_CollapseAdd_Toggle_Nấm thức dậy ngay",
            "5_CollapseAdd_Toggle_Tạm dừng nâng cao",
            "8_CollapseAdd_Spinner_<font color='green'>Đặt tốc độ game：_Tắt,1.2x,1.5x,2x,2.5x,3x,5x,10x",
            "82_CollapseAdd_OnceCheckBox_Hoàn thành màn chơi",


            "Collapse_Chức năng gỡ lỗi",
            "CollapseAdd_RichTextView_<font color='green'>Hiển thị máu cây:",
            "21_CollapseAdd_Toggle_Tất cả cây trồng",
            "22_CollapseAdd_Toggle_Cây loại chịu sát thương",
            "CollapseAdd_RichTextView_<font color='green'>Hiển thị máu zombie:",
            "23_CollapseAdd_Toggle_Máu thân thể",
            "24_CollapseAdd_Toggle_Máu giáp và khiên",
            "25_CollapseAdd_Toggle_Gargantuar",
            "CollapseAdd_RichTextView_<font color='green'>Hiển thị thông tin game:",
            "26_CollapseAdd_Toggle_Thông tin zombie xuất hiện",
            "27_CollapseAdd_Toggle_Vẽ hộp va chạm",


            "Collapse_Chức năng giải trí",
            "6_CollapseAdd_Toggle_Xóa sương mù",
            "43_CollapseAdd_Toggle_Bình trong suốt",
            "45_CollapseAdd_Toggle_Zombie không thể vào nhà",
            "44_CollapseAdd_Toggle_Wall-nut chắc chắn gây zombie phình to",
            "7_CollapseAdd_Toggle_Không rơi đồ",
            "48_CollapseAdd_Spinner_<font color='green'>Điều khiển zombie nhảy：_Tắt,Tiếp tục tiến lên,Nhảy tại chỗ,Liên tục triệu hồi",
            "46_CollapseAdd_Spinner_<font color='green'>Đường biên lùi về sau：_Tắt,10,20,30,40,50,60,70,80",
            "47_CollapseAdd_Spinner_<font color='green'>Đặt kích thước zombie：_Tắt,0.2x,0.4x,0.6x,0.8x,1x,1.2x,1.4x,1.6x,1.8x,2x",
            "CollapseAdd_RichTextView_<font color='yellow'># Lượng máu tỷ lệ bình phương với kích thước!",
            "41_CollapseAdd_OnceCheckBox_Hộp thoại mã cheat",


            "Collapse_Cài đặt màn chơi",
            "87_CollapseAdd_Toggle_Tạm dừng spawn zombie",
            "88_CollapseAdd_Toggle_Không có máy cắt cỏ",
            "83_CollapseAdd_Toggle_Không thẻ bắt buộc từ Crazy Dave",
            "84_CollapseAdd_Toggle_Last Stand vô tận",
            "89_CollapseAdd_Toggle_Vô hiệu xóa dữ liệu lưu",
            "90_CollapseAdd_Toggle_Vô hiệu lưu dữ liệu",
            "81_CollapseAdd_OnceCheckBox_Hộp thoại nhảy màn",
            "102_CollapseAdd_Spinner_<font color='green'>Đổi cảnh game：_Tắt,Ngày,Đêm,Hồ bơi,Sương mù,Mái nhà,Mái nhà đêm,Vườn Zen,Vườn nấm,Bể cá",
            "CollapseAdd_RichTextView_<font color='green'>Đặt vòng của Survival Endless:",
            "85_CollapseAdd_InputValue_Số vòng bạn muốn",
            "86_CollapseAdd_OnceCheckBox_Đặt số vòng",
            "CollapseAdd_RichTextView_<font color='yellow'># Chỉ đặt số vòng SAU KHI hoàn thành chọn thẻ!",


            "Collapse_Cài đặt khe thẻ",
            "141_CollapseAdd_Spinner_<font color='green'>Bên khe thẻ：_Khe trái,Khe phải",
            "142_CollapseAdd_Spinner_<font color='green'>Khe thẻ mục tiêu：_Thứ 1,Thứ 2,Thứ 3,Thứ 4,Thứ 5,Thứ 6,Thứ 7,Thứ 8,Thứ 9,Thứ 10",
            "143_CollapseAdd_Spinner_<font color='green'>Loại thẻ："
            "_Chưa chọn,Peashooter,Sunflower,Cherry Bomb,Wall-nut,Potato Mine,Snow Pea,Chomper,Repeater,"
            "Puff-shroom,Sun-shroom,Fume-shroom,Grave Buster,Hypno-shroom,Scaredy-shroom,Ice-shroom,Doom-shroom,"
            "Lily Pad,Squash,Threepeater,Tangle Kelp,Jalapeno,Spikeweed,Torchwood,Tall-nut,"
            "Sea-shroom,Plantern,Cactus,Blover,Split Pea,Starfruit,Pumpkin,Magnet-shroom,"
            "Cabbage-pult,Flower Pot,Kernel-pult,Coffee Bean,Garlic,Umbrella Leaf,Marigold,Melon-pult,"
            "Gatlingp Pea,Twin Sunflower,Gloom-shroom,Cattail,Winter Melon,Gold Magnet,Spikerock,Cob Cannon,"
            "Explode-o-nut,Giant Wall-nut,Sprout,Leftpeater,"
            "Grave Stone,Zombie,Trash Bin Zombie,Conehead Zombie,Pole Vaulting Zombie,Buckethead Zombie,"
            "Flag Zombie,Newspaper Zombie,Screen Door Zombie,Football Zombie,Dancing Zombie,Zomboni,"
            "Jack-in-the-box Zombie,Digger Zombie,Pogo Zombie,Bungee Zombie,Ladder Zombie,Catapult Zombie,"
            "Gargantuar,Zombie Yeti,Zombie Bobsled Team,Snorkel Zombie,Dolphin Rider Zombie,Imp,Ballon Zombie",
            "144_CollapseAdd_CheckBox_Thẻ cây Imitater",
            "145_CollapseAdd_OnceCheckBox_Thay thế thẻ",


            "Collapse_Cài đặt đạn",
            "62_CollapseAdd_Toggle_Gây sát thương mỗi khung hình",
            "61_CollapseAdd_Toggle_Snow Pea bỏ qua Torchwood",
            "63_CollapseAdd_Spinner_<font color='green'>Sửa loại đạn：_Tắt,Pea,SnowPea,Cabbage,Melon,Puff,WinterMelon,Fireball,Star,Spike,Basketball,Kernel,CobBig,Butter,Zombie Pea",
            "64_CollapseAdd_Toggle_Đạn ngẫu nhiên",
            "65_CollapseAdd_Toggle_Chỉ đổi loại đạn Pea",
            "66_CollapseAdd_Toggle_Pea đổi loại sau khi qua Torchwood",
            "67_CollapseAdd_CheckBox_Loại ngẫu nhiên loại trừ BigCob",
            "68_CollapseAdd_CheckBox_Loại ngẫu nhiên loại trừ Star",


            "Collapse_Cài đặt spawn",
            "CollapseAdd_RichTextView_<font color='green'>Chọn loại zombie:",
            "200_CollapseAdd_CheckBox_Zombie thường(Xuất hiện chắc chắn trong chế độ spawn tự nhiên)",
            "202_CollapseAdd_CheckBox_Conehead Zombie",
            "203_CollapseAdd_CheckBox_Pole Vaulting Zombie",
            "204_CollapseAdd_CheckBox_Buckethead Zombie",
            "205_CollapseAdd_CheckBox_Newspaper Zombie",
            "206_CollapseAdd_CheckBox_Screen Door Zombie",
            "207_CollapseAdd_CheckBox_Football Zombie",
            "208_CollapseAdd_CheckBox_Dancing Zombie",
            "211_CollapseAdd_CheckBox_Snorkel Zombie",
            "212_CollapseAdd_CheckBox_Zomboni",
            "214_CollapseAdd_CheckBox_Dolphin Rider Zombie",
            "215_CollapseAdd_CheckBox_Jack-in-the-box Zombie",
            "216_CollapseAdd_CheckBox_Ballon Zombie",
            "217_CollapseAdd_CheckBox_Digger Zombie",
            "218_CollapseAdd_CheckBox_Pogo Zombie",
            "219_CollapseAdd_CheckBox_Zombie Yeti",
            "220_CollapseAdd_CheckBox_Bungee Zombie",
            "221_CollapseAdd_CheckBox_Ladder Zombie",
            "222_CollapseAdd_CheckBox_Catapult Zombie",
            "223_CollapseAdd_CheckBox_Gargantuar",
            "233_CollapseAdd_CheckBox_Redeye Gargantuar",
            "226_CollapseAdd_CheckBox_Trash Bin Zombie",
            "227_CollapseAdd_CheckBox_Pea Head Zombie",
            "228_CollapseAdd_CheckBox_Wall-nut Head Zombie",
            "229_CollapseAdd_CheckBox_Jalapeno Head Zombie",
            "230_CollapseAdd_CheckBox_Gatling Zombie",
            "231_CollapseAdd_CheckBox_Squash Head Zombie",
            "232_CollapseAdd_CheckBox_Tall-nut Head Zombie",
            "234_CollapseAdd_CheckBox_All-Star Zombie",
            "235_CollapseAdd_CheckBox_Super-Fan Imp Zombie",
            "241_CollapseAdd_Spinner_<font color='green'>Chọn chế độ spawn：_Tắt,Tự nhiên (Zombie trong wave được game tự chọn),Cực độ (Điền đều zombie trong wave)",
            "242_CollapseAdd_OnceCheckBox_Đặt spawn zombie",


            "Collapse_Bố trí chiến trường",
            "101_CollapseAdd_Toggle_Tự thêm thang khi trồng Pumpkin",
            "105_CollapseAdd_Spinner_<font color='yellow'>Cột mục tiêu："
            "_Cột 1,Cột 2,Cột 3,Cột 4,Cột 5,Cột 6,Cột 7,Cột 8,Cột 9,Tất cả các cột,Vị trí zombie sinh ra",
            "106_CollapseAdd_Spinner_<font color='yellow'>Hàng mục tiêu：_Hàng 1,Hàng 2,Hàng 3,Hàng 4,Hàng 5,Hàng 6,Tất cả các hàng",
            "103_CollapseAdd_Spinner_<font color='green'>Loại cây："
            "_Chưa chọn,Peashooter,Sunflower,Cherry Bomb,Wall-nut,Potato Mine,Snow Pea,Chomper,Repeater,"
            "Puff-shroom,Sun-shroom,Fume-shroom,Grave Buster,Hypno-shroom,Scaredy-shroom,Ice-shroom,Doom-shroom,"
            "Lily Pad,Squash,Threepeater,Tangle Kelp,Jalapeno,Spikeweed,Torchwood,Tall-nut,"
            "Sea-shroom,Plantern,Cactus,Blover,Split Pea,Starfruit,Pumpkin,Magnet-shroom,"
            "Cabbage-pult,Flower Pot,Kernel-pult,Coffee Bean,Garlic,Umbrella Leaf,Marigold,Melon-pult,"
            "Gatlingp Pea,Twin Sunflower,Gloom-shroom,Cattail,Winter Melon,Gold Magnet,Spikerock,Cob Cannon,"
            "Explode-o-nut,Giant Wall-nut,Sprout,Leftpeater",
            "107_CollapseAdd_CheckBox_Cây Imitater",
            "108_CollapseAdd_OnceCheckBox_Thêm cây",
            "104_CollapseAdd_Spinner_<font color='green'>Loại zombie："
            "_Chưa chọn,Zombie,Flag Zombie,Conehead Zombie,Pole Vaulting Zombie,Buckethead Zombie,"
            "Newspaper Zombie,Screen Door Zombie,Football Zombie,Dancing Zombie,Backup Dancer,"
            "Ducky Tube Zombie,Snorkel Zombie,Zomboni,Zombie Bobsled Team,Dolphin Rider Zombie,"
            "Jack-in-the-box Zombie,Ballon Zombie,Digger Zombie,Pogo Zombie,Zombie Yeti,"
            "Bungee Zombie,Ladder Zombie,Catapult Zombie,Gargantuar,Imp,Dr. Zomboss,"
            "Trash Bin Zombie,Pea Head Zombie,Wall-nut Head Zombie,Jalapeno Head Zombie,Gatling Zombie,Squash Head Zombie,Tall-nut Head Zombie,Redeye Gargantuar,"
            "All-Star Zombie, Super-Fan Imp Zombie",
            "109_CollapseAdd_OnceCheckBox_Thêm zombie",
            "CollapseAdd_RichTextView_<font color='green'>Đối tượng khác:",
            "114_CollapseAdd_OnceCheckBox_Thêm mộ đá",
            "110_CollapseAdd_OnceCheckBox_Thêm thang",
            "111_CollapseAdd_OnceCheckBox_Khôi phục tất cả máy cắt cỏ",
            "CollapseAdd_RichTextView_<font color='yellow'>Xóa bỏ:",
            "112_CollapseAdd_OnceCheckBox_Xóa tất cả cây",
            "115_CollapseAdd_OnceCheckBox_Xóa tất cả zombie",
            "116_CollapseAdd_OnceCheckBox_Xóa tất cả mộ đá",
            "113_CollapseAdd_OnceCheckBox_Xóa tất cả máy cắt cỏ",
            "CollapseAdd_RichTextView_<font color='yellow'>Đặt trạng thái:",
            "9_CollapseAdd_OnceCheckBox_Thôi miên tất cả zombie",
            "10_CollapseAdd_OnceCheckBox_Đóng băng tất cả zombie",
            "11_CollapseAdd_OnceCheckBox_Kích hoạt tất cả máy cắt cỏ",


            "Collapse_Bố trí nhanh",
            "121_CollapseAdd_Spinner_<font color='green'>Chọn đội hình cho Pool：_Chưa chọn,"
            "[0]Radio Clock Cobless,[1]Minimalist Cobless,[2]Pseudo-Unharmed Cobless,[3]Automatically Subdue Jack-in-the-box Cobless,[4]Fiery Cobless,"
            "[5]Split Fiery Cobless,[6]Rear Cobless,[7]Super Forward Cobless,[8]Prince Cobless,[9]Mechanical Clock Cobless,"
            "[10]Sideless Cobless,[11]Quartz Clock Cobless,[12]Sunflowerless Cobless,[13]Square Gloomless Cobless,[14]56 Row Accelerated Gloomless Cobless,"
            "[15]1 Cob Suppression,[16]Little ② Cob,[17]Fiery 2 Cob,[18]Nuclear 2 Cob,[19]Split 2 Cob,"
            "[20]Square 2 Cob,[21]Classic 2 Cob,[22]Speedrun 3 Cob,[23]Tai Chi 4 Cob,[24]Metallic 4 Cob,"
            "[25]Square 4 Cob,[26]Verdant 4 Cob,[27]Waterless 4 Cob,[28]Diamond 4 Cob,[29]Sideless 4 Cob,"
            "[30]Nuclear Back 4 Cob,[31]Classic 4 Cob,[32]Fiery 4 Cob,[33]Back 4 Cob,[34]Traditional 4 Cob,"
            "[35]Half 5 Cob,[36]Scattershot 5 Cob,[37]Heart 5 Cob,[38]Landless 6 Cob,[39]Waterless 6 Cob,"
            "[40]Verdant 6 Cob,[41]Zen Room Deep in Flowers and Plants,[42]Sideless 6 Cob,[43]Kernel 6 Cob,[44]Air Bomb 6 Cob,"
            "[45]Super Rear 6 Cob,[46]Square 6 Cob,[47]Butterfly Rhyme,[48]A Spoonful of Sweet Soup Balls,[49]Separated 7 Cob,"
            "[50]Jade Rabbit,[51]Pumpkinless 8 Cob,[52]Christmas Tree 8 Cob,[53]Centered Christmas Tree 8 Cob,[54]Rectangular 8 Cob,"
            "[55]Sideless 8 Cob,[56]Yin and Yang 8 Cob,[57]Duckweed 8 Cob,[58]Rear 8 Cob,[59]To Feed a Dolphin,"
            "[60]Kernel 8 Cob,[61]Classic 8 Cob,[62]Sea of Flowers 8 Cob,[63]C2 8 Cob,[64]Separation 8 Cob,"
            "[65]Full-Symmetrical 8 Cob,[66]3C 8 Cob,[67]Lampstand 8 Cob,[68]⑨ Cob,[69]Square 9 Cob,"
            "[70]C6i 9 Cob,[71]Heart 9 Cob,[72]Alternate 9 Cob,[73]② Cob,[74]Hexagram 10 Cob,"
            "[75]Hexagon 10 Cob,[76]Square 10 Cob,[77]Diamond 10 Cob,[78]Simplified 10 Cob,[79]Rear 10 Cob,"
            "[80]Classic 10 Cob,[81]To Imprison 6 Zombies,[82]Diagonal 10 Cob,[83]Rubik's Cube 10 Cob,[84]Dave's Hamburger,"
            "[85]Cocktail,[86]A Spoonful of Sweet Soup Balls 12 Cob,[87]Porcelain Vase 12 Cob,[88]Half 12 Cob,[89]Simplified 12 Cob,"
            "[90]Classic 12 Cob,[91]Fiery 12 Cob,[92]Ice Rain 12 Cob (remodelled),[93]Ice Rain 12 Cob (remodelled x2),[94]Single Purple Card 12 Cob,"
            "[95]Sideless Column 12 Cob,[96]Sideless 12 Cob,[97]Waterless 12 Cob,[98]Pure White Card Impending 12 Cob,[99]Backyard 12 Cob,"
            "[100]Kernel 8 Cob,[101]Two Row Accelerated 12 Cob,[102]Front 12 Cob,[103]Ladder & Gloom-shroom 12 Cob,[104]Jun Hai 12 Cob,"
            "[105]A Poem for The Harp,[106]Plum Blossom 13 Cob,[107]Last Order,[108]Ice Lantern 14 Cob,[109]Tai Chi 14 Cob,"
            "[110]Real ④ Cob,[111]God Stick 14 Cob,[112]Sideless 14 Cob,[113]Timeslip 14 Cob,[114]Diamond 15 Cob,"
            "[115]Sideless 15 Cob,[116]Real ② Cob,[117]Icebox 16 Cob,[118]Cob Ring 12 Sunflower,[119]Single Ice 16 Cob,"
            "[120]Symmetrical 16 Cob,[121]Sideless 16 Cob,[122]Streaking 16 Cob,[123]Double Ice 16 Cob,[124]Super Forward 16 Cob,"
            "[125]Fiery 16 Cob,[126]Classic 16 Cob,[127]Zigzag Line 16 Cob,[128]Lung 18 Cob (extreme),[129]Pure 18 Cob,"
            "[130]Real 18 Cob,[131]Ice Soul 18 Cob,[132]Tail Bomb 18 Cob,[133]Classic 18 Cob,[134]Pure 20 Cob,"
            "[135]Air Bomb 20 Cob,[136]Rake 20 Cob,[137]New 20 Cob,[138]Winter-melonless 20 Cob,[139]The Road of Despair,"
            "[140]21 Cob,[141]New 22 Cob,[142]22 Cob,[143]Winter-melonless 22 Cob,[144]Front 22 Cob,"
            "[145]24 Cob,[146]Cannon Fodder 24 Cob,[147]Scaredy-shroom Protecter (extreme)",
            "122_CollapseAdd_OnceCheckBox_Triển khai đội hình",
            "CollapseAdd_RichTextView_<font color='green'>Xuất/Nhập đội hình:",
            "123_CollapseAdd_FormationCopy_Sao chép mã đội hình",
            "124_CollapseAdd_InputText_Dán mã đội hình",
            "125_CollapseAdd_OnceCheckBox_Triển khai đội hình",
            "CollapseAdd_RichTextView_<font color='yellow'># Bạn có thể triển khai đội hình khi đang tạm dừng",
        };

        static_assert(CheckList(settingsList));
        static_assert(CheckList(featureList));
    } // namespace vi

    namespace zh_Hans {
        inline constexpr SettingsList settingsList = {
            "Category_菜单设置",
            "-1_Toggle_退出时保存设置", //-1 is checked on Preferences.java
            "Category_更多",
            "-6_Button_<font color='green'>返回菜单</font>",
        };

        inline constexpr FeatureList featureList = {
            "Collapse_常用功能",
            "1_CollapseAdd_Toggle_无限阳光",
            "42_CollapseAdd_Toggle_自由种植",
            "2_CollapseAdd_Toggle_卡片无冷却",
            "3_CollapseAdd_Toggle_技能无冷却",
            "4_CollapseAdd_Toggle_蘑菇免唤醒",
            "5_CollapseAdd_Toggle_高级暂停",
            "8_CollapseAdd_Spinner_<font color='green'>设置游戏倍速：_关闭,1.2倍,1.5倍,2倍,2.5倍,3倍,5倍,10倍",
            "82_CollapseAdd_OnceCheckBox_直接过关",


            "Collapse_调试功能",
            "CollapseAdd_RichTextView_<font color='green'>显示植物血量:",
            "21_CollapseAdd_Toggle_所有植物显血",
            "22_CollapseAdd_Toggle_损伤点类植物显血",
            "CollapseAdd_RichTextView_<font color='green'>显示僵尸血量:",
            "23_CollapseAdd_Toggle_本体显血",
            "24_CollapseAdd_Toggle_防具显血",
            "25_CollapseAdd_Toggle_巨人显血",
            "CollapseAdd_RichTextView_<font color='green'>显示游戏信息:",
            "26_CollapseAdd_Toggle_显示出怪信息",
            "27_CollapseAdd_Toggle_绘制碰撞箱",


            "Collapse_娱乐功能",
            "6_CollapseAdd_Toggle_清除迷雾",
            "43_CollapseAdd_Toggle_罐子透视",
            "45_CollapseAdd_Toggle_无视僵尸进家",
            "44_CollapseAdd_Toggle_普僵啃坚果必过敏",
            "7_CollapseAdd_Toggle_禁止掉落阳光金币",
            "48_CollapseAdd_Spinner_<font color='green'>女仆秘籍：_关闭,保持前进(舞王一直前进),跳舞(舞王不前进且不会召唤舞伴),召唤舞伴(舞王不前进且一直尝试召唤舞伴)",
            "46_CollapseAdd_Spinner_<font color='green'>进家线后退：_关闭,10,20,30,40,50,60,70,80",
            "47_CollapseAdd_Spinner_<font color='green'>修改僵尸大小：_关闭,0.2,0.4,0.6,0.8,1.0,1.2,1.4,1.6,1.8,2.0",
            "CollapseAdd_RichTextView_<font color='yellow'>#血量与大小呈平方关系!",
            "41_CollapseAdd_OnceCheckBox_智慧树指令",


            "Collapse_关卡设置",
            "87_CollapseAdd_Toggle_暂停刷怪",
            "88_CollapseAdd_Toggle_不出小推车",
            "83_CollapseAdd_Toggle_取消必选卡片",
            "84_CollapseAdd_Toggle_坚不可摧无尽",
            "89_CollapseAdd_Toggle_禁止删档",
            "90_CollapseAdd_Toggle_禁止存档",
            "81_CollapseAdd_OnceCheckBox_跳关器",
            "102_CollapseAdd_Spinner_<font color='green'>更换场景：_关闭,白天,黑夜,泳池,雾夜,屋顶,月夜,温室园,蘑菇园,水族馆",
            "CollapseAdd_RichTextView_<font color='green'>无尽模式跳轮:",
            "85_CollapseAdd_InputValue_目标轮数",
            "86_CollapseAdd_OnceCheckBox_跳轮",
            "CollapseAdd_RichTextView_<font color='yellow'>#完成选卡后才能跳轮!",


            "Collapse_卡槽设置",
            "141_CollapseAdd_Spinner_<font color='green'>修改卡槽：_卡槽1,卡槽2",
            "142_CollapseAdd_Spinner_<font color='green'>卡片位置：_第1格,第2格,第3格,第4格,第5格,第6格,第7格,第8格,第9格,第10格",
            // "143_CollapseAdd_Spinner_<font
            // color='green'>全部卡片类型：_关闭,豌豆射手,向日葵,樱桃炸弹,坚果,土豆地雷,寒冰射手,大嘴花,双重射手,小喷菇,阳光菇,大喷菇,咬咬碑,迷惑菇,胆小菇,冰川菇,末日菇,莲叶,窝瓜,三重射手,缠绕水草,火爆辣椒,地刺,火炬树桩,高坚果,水兵菇,路灯花,仙人掌,三叶草,双向射手,星星果,南瓜头,磁力菇,卷心菜投手,花盆,玉米投手,咖啡豆,大蒜,叶子保护伞,金盏花,西瓜投手,机枪射手,双胞向日葵,多嘴小蘑菇,猫尾草,冰西瓜,吸金磁,钢地刺,玉米加农炮,变身茄子,无(可选植物种类总数),爆炸坚果,巨型坚果,幼苗,反向双重射手,无(植物种类总数),刷新阵型(僵尸迷阵),消除炸弹坑(僵尸迷阵),阳光(拉霸),钻石(拉霸),潜水僵尸(僵尸水族馆),奖杯(僵尸水族馆),墓碑,普通僵尸,垃圾桶僵尸,路障僵尸,撑杆僵尸,铁桶僵尸,旗帜僵尸,报纸僵尸,铁网门僵尸,橄榄球僵尸,舞者僵尸,雪橇车僵尸,玩偶匣僵尸,矿工僵尸,蹦蹦僵尸,蹦极僵尸,梯子僵尸,投石车僵尸,巨人僵尸,无,鸭子救生圈僵尸,潜水僵尸,海豚骑士僵尸,小鬼僵尸,气球僵尸",
            "143_CollapseAdd_Spinner_<font "
            "color='green'>卡片类型："
            "_未选择,豌豆射手,向日葵,樱桃炸弹,坚果,土豆地雷,寒冰射手,大嘴花,双重射手,"
            "小喷菇,阳光菇,大喷菇,咬咬碑,迷惑菇,胆小菇,冰川菇,末日菇,"
            "莲叶,窝瓜,三重射手,缠绕水草,火爆辣椒,地刺,火炬树桩,高坚果,"
            "水兵菇,路灯花,仙人掌,三叶草,双向射手,星星果,南瓜头,磁力菇,"
            "卷心菜投手,花盆,玉米投手,咖啡豆,大蒜,叶子保护伞,金盏花,西瓜投手,"
            "机枪射手,双胞向日葵,多嘴小蘑菇,猫尾草,冰西瓜,吸金磁,钢地刺,玉米加农炮,"
            "爆炸坚果,巨型坚果,幼苗,反向双重射手,"
            "墓碑,普通僵尸,垃圾桶僵尸,路障僵尸,撑杆僵尸,铁桶僵尸,"
            "旗帜僵尸,报纸僵尸,铁网门僵尸,橄榄球僵尸,舞者僵尸,雪橇车僵尸,"
            "玩偶匣僵尸,矿工僵尸,蹦蹦僵尸,蹦极僵尸,梯子僵尸,投石车僵尸,"
            "巨人僵尸,僵尸雪人,雪橇车僵尸小队,潜水僵尸,海豚骑士僵尸,小鬼僵尸,气球僵尸",
            "144_CollapseAdd_CheckBox_模仿者植物卡片",
            "145_CollapseAdd_OnceCheckBox_更换卡片",


            "Collapse_子弹设置",
            "62_CollapseAdd_Toggle_子弹帧伤",
            "61_CollapseAdd_Toggle_寒冰豌豆无视火炬",
            "63_CollapseAdd_Spinner_<font color='green'>手动选择子弹类型：_关闭,豌豆,寒冰豌豆,卷心菜,西瓜,蘑菇孢子,冰西瓜,火焰豌豆,星星,尖刺,篮球,玉米粒,玉米炮弹,黄油,自残子弹",
            "64_CollapseAdd_Toggle_随机子弹类型",
            "65_CollapseAdd_Toggle_仅对普通豌豆生效",
            "66_CollapseAdd_Toggle_豌豆穿过火炬后转变",
            "67_CollapseAdd_CheckBox_Ban玉米炮弹",
            "68_CollapseAdd_CheckBox_Ban星星子弹",


            "Collapse_出怪设置",
            "CollapseAdd_RichTextView_<font color='green'>请选择僵尸种类:",
            "200_CollapseAdd_CheckBox_普通僵尸(自然出怪必出)",
            "202_CollapseAdd_CheckBox_路障僵尸",
            "203_CollapseAdd_CheckBox_撑杆僵尸",
            "204_CollapseAdd_CheckBox_铁桶僵尸",
            "205_CollapseAdd_CheckBox_报纸僵尸",
            "206_CollapseAdd_CheckBox_铁网门僵尸",
            "207_CollapseAdd_CheckBox_橄榄球僵尸",
            "208_CollapseAdd_CheckBox_舞者僵尸",
            "211_CollapseAdd_CheckBox_潜水僵尸",
            "212_CollapseAdd_CheckBox_雪橇车僵尸",
            "214_CollapseAdd_CheckBox_海豚骑士僵尸",
            "215_CollapseAdd_CheckBox_玩偶匣僵尸",
            "216_CollapseAdd_CheckBox_气球僵尸",
            "217_CollapseAdd_CheckBox_矿工僵尸",
            "218_CollapseAdd_CheckBox_蹦蹦僵尸",
            "219_CollapseAdd_CheckBox_僵尸雪人",
            "220_CollapseAdd_CheckBox_蹦极僵尸",
            "221_CollapseAdd_CheckBox_梯子僵尸",
            "222_CollapseAdd_CheckBox_投石车僵尸",
            "223_CollapseAdd_CheckBox_白眼巨人僵尸",
            "233_CollapseAdd_CheckBox_红眼巨人僵尸",
            "226_CollapseAdd_CheckBox_垃圾桶僵尸",
            "227_CollapseAdd_CheckBox_豌豆射手僵尸",
            "228_CollapseAdd_CheckBox_坚果僵尸",
            "229_CollapseAdd_CheckBox_火爆辣椒僵尸",
            "230_CollapseAdd_CheckBox_机枪射手僵尸",
            "231_CollapseAdd_CheckBox_窝瓜僵尸",
            "232_CollapseAdd_CheckBox_高坚果僵尸",
            "234_CollapseAdd_CheckBox_全明星僵尸",
            "235_CollapseAdd_CheckBox_粉丝小鬼僵尸",
            "241_CollapseAdd_Spinner_<font color='green'>请选择刷怪模式：_关闭,自然出怪(由游戏生成出怪列表),极限出怪(均匀填充出怪列表)",
            "242_CollapseAdd_OnceCheckBox_设置出怪",


            "Collapse_场地布置",
            "101_CollapseAdd_Toggle_种下南瓜自动搭梯",
            "105_CollapseAdd_Spinner_<font color='yellow'>放置横坐标：_第1列,第2列,第3列,第4列,第5列,第6列,第7列,第8列,第9列,所有列,僵尸出生点",
            "106_CollapseAdd_Spinner_<font color='yellow'>放置纵坐标：_第1行,第2行,第3行,第4行,第5行,第6行,所有行",
            "103_CollapseAdd_Spinner_<font color='green'>植物类型："
            "_未选择,豌豆射手,向日葵,樱桃炸弹,坚果,土豆地雷,寒冰射手,大嘴花,双重射手,"
            "小喷菇,阳光菇,大喷菇,咬咬碑,迷惑菇,胆小菇,冰川菇,末日菇,"
            "莲叶,窝瓜,三重射手,缠绕水草,火爆辣椒,地刺,火炬树桩,高坚果,"
            "水兵菇,路灯花,仙人掌,三叶草,双向射手,星星果,南瓜头,磁力菇,"
            "卷心菜投手,花盆,玉米投手,咖啡豆,大蒜,叶子保护伞,金盏花,西瓜投手,"
            "机枪射手,双胞向日葵,多嘴小蘑菇,猫尾草,冰西瓜,吸金磁,钢地刺,玉米加农炮,"
            "爆炸坚果,巨型坚果,幼苗,反向双重射手",
            "107_CollapseAdd_CheckBox_模仿者植物",
            "108_CollapseAdd_OnceCheckBox_种植植物",
            "104_CollapseAdd_Spinner_<font "
            "color='green'>僵尸类型："
            "_未选择,普通僵尸,旗帜僵尸,路障僵尸,撑杆僵尸,铁桶僵尸,"
            "报纸僵尸,铁网门僵尸,橄榄球僵尸,舞者僵尸,伴舞僵尸,"
            "鸭子救生圈僵尸,潜水僵尸,雪橇车僵尸,雪橇车僵尸小队,海豚骑士僵尸,"
            "玩偶匣僵尸,气球僵尸,矿工僵尸,蹦蹦僵尸,僵尸雪人,"
            "蹦极僵尸,梯子僵尸,投石车僵尸,白眼巨人僵尸,小鬼僵尸,僵王博士,"
            "垃圾桶僵尸,豌豆射手僵尸,坚果僵尸,火爆辣椒僵尸,机枪射手僵尸,窝瓜僵尸,高坚果僵尸,红眼巨人僵尸,"
            "全明星僵尸,粉丝小鬼僵尸",
            "109_CollapseAdd_OnceCheckBox_放置僵尸",
            "CollapseAdd_RichTextView_<font color='green'>其他类型:",
            "114_CollapseAdd_OnceCheckBox_放置墓碑",
            "110_CollapseAdd_OnceCheckBox_放置梯子",
            "111_CollapseAdd_OnceCheckBox_恢复所有小推车",
            "CollapseAdd_RichTextView_<font color='yellow'>清除:",
            "112_CollapseAdd_OnceCheckBox_清除所有植物",
            "115_CollapseAdd_OnceCheckBox_清除所有僵尸",
            "116_CollapseAdd_OnceCheckBox_清除所有墓碑",
            "113_CollapseAdd_OnceCheckBox_清除所有小推车",
            "CollapseAdd_RichTextView_<font color='yellow'>杂项:",
            "9_CollapseAdd_OnceCheckBox_魅惑所有僵尸",
            "10_CollapseAdd_OnceCheckBox_冻结所有僵尸",
            "11_CollapseAdd_OnceCheckBox_启动所有小推车",


            "Collapse_一键布阵",
            "121_CollapseAdd_Spinner_<font "
            "color='green'>选择泳池无尽阵型：_未选择,"
            "[0]电波钟无炮,[1]最简无炮,[2]伪无伤无炮,[3]自然控丑无炮,[4]火焰无炮,[5]分裂火焰无炮,[6]后退无炮,[7]超前置无炮,[8]王子无炮,[9]机械钟无炮,"
            "[10]神之无炮,[11]石英钟无炮,[12]靠天无炮,[13]方块无神无炮,[14]56加速无神无炮,[15]压制一炮,[16]小二炮,[17]火焰二炮,[18]核武二炮,[19]分裂二炮,"
            "[20]方正二炮,[21]经典二炮,[22]冲关三炮,[23]太极四炮,[24]全金属四炮,[25]方块四炮,[26]青四炮,[27]水路无植物四炮,[28]方四炮,[29]神之四炮,"
            "[30]双核底线四炮,[31]经典四炮,[32]火焰四炮,[33]底线四炮,[34]传统四炮,[35]半场无植物五炮,[36]散炸五炮,[37]心五炮,[38]陆路无植物六炮,[39]水路无植物六炮,"
            "[40]青苔六炮,[41]禅房花木深,[42]神之六炮,[43]玉米六炮,[44]空炸六炮,[45]超后置六炮,[46]方六炮,[47]蝶韵,[48]一勺汤圆,[49]间隔无植物七炮,"
            "[50]玉兔茕茕,[51]无保护八炮,[52]树八炮,[53]全对称树八炮,[54]矩形八炮,[55]神之八炮,[56]阴阳八炮,[57]浮萍八炮,[58]后置八炮,[59]饲养海豚,"
            "[60]玉米八炮,[61]经典八炮,[62]花海八炮,[63]C2八炮,[64]分离八炮,[65]全对称八炮,[66]3C八炮,[67]灯台八炮,[68]⑨炮,[69]方块九炮,"
            "[70]C6i九炮,[71]心九炮,[72]轮炸九炮,[73]②炮,[74]六芒星十炮,[75]六边形十炮,[76]方块十炮,[77]斜方十炮,[78]简化十炮,[79]后置十炮,"
            "[80]经典十炮,[81]六线囚尸,[82]斜十炮,[83]魔方十炮,[84]戴夫的小汉堡,[85]鸡尾酒,[86]一勺汤圆十二炮,[87]玉壶春十二炮,[88]半场十二炮,[89]简化十二炮,"
            "[90]经典十二炮,[91]火焰十二炮,[92]冰雨十二炮·改,[93]冰雨十二炮•改改,[94]单紫卡十二炮,[95]神柱十二炮,[96]神之十二炮,[97]水路无植物十二炮,[98]纯白悬空十二炮,[99]后花园十二炮,"
            "[100]玉米十二炮,[101]两路暴狂,[102]九列十二炮,[103]梯曾十二炮,[104]君海十二炮,[105]箜篌引,[106]梅花十三,[107]最后之作,[108]冰心灯,[109]太极十四炮,"
            "[110]真·四炮,[111]神棍十四炮,[112]神之十四炮,[113]穿越十四炮,[114]钻石十五炮,[115]神之十五炮,[116]真·二炮,[117]冰箱灯,[118]炮环十二花,[119]单冰十六炮,"
            "[120]对称十六炮,[121]神之十六炮,[122]裸奔十六炮,[123]双冰十六炮,[124]超前置十六炮,[125]火焰十六炮,[126]经典十六炮,[127]折线十六炮,[128]肺十八炮(极限),[129]纯十八炮,"
            "[130]真·十八炮,[131]冰魄十八炮,[132]尾炸十八炮,[133]经典十八炮,[134]纯二十炮,[135]空炸二十炮,[136]钉耙二十炮,[137]新二十炮,[138]无冰瓜二十炮,[139]绝望之路,"
            "[140]二十一炮,[141]新二十二炮,[142]二十二炮,[143]无冰瓜二十二炮,[144]九列二十二炮,[145]二十四炮,[146]垫材二十四炮,[147]胆守(极限)",
            "122_CollapseAdd_OnceCheckBox_布置已选阵型",
            "CollapseAdd_RichTextView_<font color='green'>阵型导出/导入:",
            "123_CollapseAdd_FormationCopy_复制阵型代码",
            "124_CollapseAdd_InputText_粘贴阵型代码",
            "125_CollapseAdd_OnceCheckBox_布置粘贴阵型",
            "CollapseAdd_RichTextView_<font color='yellow'>#可以暂停游戏后进行布阵",
        };

        static_assert(CheckList(settingsList));
        static_assert(CheckList(featureList));
    } // namespace zh_Hans

} // namespace lang

} // namespace cheat

#endif // PVZ_CHEAT_H
