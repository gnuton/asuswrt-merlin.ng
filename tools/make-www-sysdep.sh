#!/bin/bash
# Run me from within www/sysdep/MODEL_NAME/www/

rm WiFi_Insight.asp
rm Main_Netstat_Content.asp
rm Main_Analysis_Content.asp

ln -s ../../FUNCTION/ROG_UI/Main_Netstat_Content.asp Main_Netstat_Content.asp
#ln -s ../../FUNCTION/ROG_UI/WiFi_Insight.asp WiFi_Insight.asp
ln -s ../../FUNCTION/ROG_UI/Main_Analysis_Content.asp Main_Analysis_Content.asp

#rm images/New_ui/wifi_setting.png  images/New_ui/wifi_statistic.png  images/New_ui/wifi_survey.png  images/New_ui/wifi_troubleShooting.png
#ln -s ../../../../FUNCTION/ROG_UI/images/New_ui/wifi_setting.png images/New_ui/wifi_setting.png
#ln -s ../../../../FUNCTION/ROG_UI/images/New_ui/wifi_statistic.png images/New_ui/wifi_statistic.png
#ln -s ../../../../FUNCTION/ROG_UI/images/New_ui/wifi_survey.png images/New_ui/wifi_survey.png
#ln -s ../../../../FUNCTION/ROG_UI/images/New_ui/wifi_troubleShooting.png images/New_ui/wifi_troubleShooting.png
