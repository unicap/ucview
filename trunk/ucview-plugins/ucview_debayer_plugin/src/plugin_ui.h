#ifndef __PLUGIN_UI_H__
#define __PLUGIN_UI_H__
static char* plugin_ui ="<?xml version='1.0'?>"\
"<!--Generated with glade3 3.4.5 on Tue Nov 25 07:19:17 2008 -->"\
"<interface>"\
"  <object class='GtkAdjustment' id='adjustment1'>"\
"    <property name='upper'>6</property>"\
"    <property name='lower'>0</property>"\
"    <property name='page_increment'>0.5</property>"\
"    <property name='step_increment'>0.01</property>"\
"    <property name='page_size'>1</property>"\
"    <property name='value'>1</property>"\
"  </object>"\
"  <object class='GtkAdjustment' id='adjustment2'>"\
"    <property name='upper'>6</property>"\
"    <property name='lower'>0</property>"\
"    <property name='page_increment'>0.5</property>"\
"    <property name='step_increment'>0.01</property>"\
"    <property name='page_size'>1</property>"\
"    <property name='value'>1</property>"\
"  </object>"\
"  <object class='GtkListStore' id='model1'>"\
"    <columns>"\
"      <column type='gchararray'/>"\
"    </columns>"\
"    <data>"\
"      <row>"\
"        <col id='0' translatable='yes'>Nearest</col>"\
"      </row>"\
"      <row>"\
"        <col id='0' translatable='yes'>Edge Sensing</col>"\
"      </row>"\
"    </data>"\
"  </object>"\
"  <object class='GtkWindow' id='window1'>"\
"    <child>"\
"      <object class='GtkVBox' id='vbox1'>"\
"        <property name='visible'>True</property>"\
"        <child>"\
"          <object class='GtkHBox' id='hbox1'>"\
"            <property name='visible'>True</property>"\
"            <child>"\
"              <object class='GtkLabel' id='label1'>"\
"                <property name='visible'>True</property>"\
"                <property name='label' translatable='yes'>Interpolation Type:</property>"\
"              </object>"\
"              <packing>"\
"                <property name='expand'>False</property>"\
"                <property name='fill'>False</property>"\
"                <property name='padding'>6</property>"\
"              </packing>"\
"            </child>"\
"            <child>"\
"              <object class='GtkComboBox' id='combobox1'>"\
"                <property name='visible'>True</property>"\
"                <signal handler='interp_type_combo_changed' name='changed'/>"\
"                <property name='model'>model1</property>"\
"                <child>"\
"                  <object class='GtkCellRendererText' id='renderer1'/>"\
"                  <attributes>"\
"                    <attribute name='text'>0</attribute>"\
"                  </attributes>"\
"                </child>"\
"              </object>"\
"              <packing>"\
"                <property name='position'>1</property>"\
"              </packing>"\
"            </child>"\
"          </object>"\
"        </child>"\
"        <child>"\
"          <object class='GtkCheckButton' id='activate_ccm_check'>"\
"            <property name='visible'>True</property>"\
"            <property name='can_focus'>True</property>"\
"            <property name='label' translatable='yes'>Enable Colour Correction Matrix</property>"\
"            <property name='draw_indicator'>True</property>"\
"            <signal handler='activate_ccm' name='toggled'/>"\
"          </object>"\
"          <packing>"\
"            <property name='expand'>False</property>"\
"            <property name='fill'>False</property>"\
"            <property name='position'>1</property>"\
"          </packing>"\
"        </child>"\
"        <child>"\
"          <object class='GtkCheckButton' id='activate_rbgain_check'>"\
"            <property name='visible'>True</property>"\
"            <property name='can_focus'>True</property>"\
"            <property name='label' translatable='yes'>Enable Red/Blue Gain ( Whitebalance )</property>"\
"            <property name='draw_indicator'>True</property>"\
"            <signal handler='activate_rbgain' name='toggled'/>"\
"          </object>"\
"          <packing>"\
"            <property name='position'>2</property>"\
"          </packing>"\
"        </child>"\
"        <child>"\
"          <object class='GtkHBox' id='hbox2'>"\
"            <property name='visible'>True</property>"\
"            <property name='spacing'>4</property>"\
"            <child>"\
"              <object class='GtkVBox' id='vbox2'>"\
"                <property name='visible'>True</property>"\
"                <child>"\
"                  <object class='GtkCheckButton' id='activate_auto_rbgain_check'>"\
"                    <property name='visible'>True</property>"\
"                    <property name='can_focus'>True</property>"\
"                    <property name='label' translatable='yes'>Auto</property>"\
"                    <property name='draw_indicator'>True</property>"\
"                    <signal handler='activate_auto_rbgain' name='toggled'/>"\
"                  </object>"\
"                </child>"\
"                <child>"\
"                  <object class='GtkHBox' id='hbox3'>"\
"                    <property name='visible'>True</property>"\
"                    <child>"\
"                      <object class='GtkLabel' id='label'>"\
"                        <property name='visible'>True</property>"\
"                        <property name='label' translatable='yes'>Red Gain:</property>"\
"                      </object>"\
"                      <packing>"\
"                        <property name='expand'>False</property>"\
"                        <property name='fill'>False</property>"\
"                      </packing>"\
"                    </child>"\
"                    <child>"\
"                      <object class='GtkHScale' id='rscale'>"\
"                        <property name='visible'>True</property>"\
"                        <property name='can_focus'>True</property>"\
"                        <property name='adjustment'>adjustment1</property>"\
"                        <property name='digits'>2</property>"\
"                        <signal handler='rgain_changed' name='value_changed'/>"\
"                      </object>"\
"                      <packing>"\
"                        <property name='position'>1</property>"\
"                      </packing>"\
"                    </child>"\
"                  </object>"\
"                  <packing>"\
"                    <property name='position'>1</property>"\
"                  </packing>"\
"                </child>"\
"                <child>"\
"                  <object class='GtkHBox' id='hbox4'>"\
"                    <property name='visible'>True</property>"\
"                    <child>"\
"                      <object class='GtkLabel' id='label2'>"\
"                        <property name='visible'>True</property>"\
"                        <property name='label' translatable='yes'>Blue Gain:</property>"\
"                      </object>"\
"                      <packing>"\
"                        <property name='expand'>False</property>"\
"                        <property name='fill'>False</property>"\
"                      </packing>"\
"                    </child>"\
"                    <child>"\
"                      <object class='GtkHScale' id='bscale'>"\
"                        <property name='visible'>True</property>"\
"                        <property name='can_focus'>True</property>"\
"                        <property name='adjustment'>adjustment2</property>"\
"                        <property name='digits'>2</property>"\
"                        <signal handler='bgain_changed' name='value_changed'/>"\
"                      </object>"\
"                      <packing>"\
"                        <property name='position'>1</property>"\
"                      </packing>"\
"                    </child>"\
"                  </object>"\
"                  <packing>"\
"                    <property name='position'>2</property>"\
"                  </packing>"\
"                </child>"\
"              </object>"\
"              <packing>"\
"                <property name='padding'>12</property>"\
"              </packing>"\
"            </child>"\
"          </object>"\
"          <packing>"\
"            <property name='position'>3</property>"\
"          </packing>"\
"        </child>"\
"      </object>"\
"    </child>"\
"  </object>"\
"</interface>"\
"";
#endif//__PLUGIN_UI_H__