/* Tests
 * Copyright (C) 2005-2007 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */

namespace {     // Anon

const char *xml_data1 = /* random, but valid xml data */
"<?xml version='1.0' encoding='UTF-8'?>		<!--*-mode: xml;-*-->\n"
"<toplevel-tag\n"
"  xmlns:xfoo='http://beast.gtk.org/xmlns:xfoo' >\n"
"\n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"  <!-- test nodes used for assertions -->\n"
"  <child1 a='a' b='1234b' c='_&lt;-&gt;^&amp;+;'/>\n"
"  <child2><x/><y/><z/><child3/><A/><B/><C/><x/><y/><z/></child2>\n"
"  <child4>foo-blah</child4>\n"
"  <child5>foo<x>bar</x>basel<y tag='bulu'>droehn<markup>!</markup></y></child5>\n"
"  <orderedattribs x1='A' z='B' U='C' foofoo='D' coffee='E' last='F'>orderedattribs-text</orderedattribs>\n"
"  <!-- test nodes used for assertions end here -->\n"
"  <!-- a real comment -->\n"
"  <xfoo:zibtupasti    inherit='xxx1' />\n"
"  <xfoo:ohanifurtasy  inherit='YbbmXXX1' />\n"
"  <xfoo:chacka        inherit='XyXyXyXy'\n"
"    prop:orientation='$(somestring)'\n"
"    prop:x-channels='$(other args,$n,0)' />\n"
"  <xfoo:ergostuff     inherit='Parent' >\n"
"  </xfoo:ergostuff>\n"
"\n"
"  <tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/><tag/>"
"  <!-- comment2 -->\n"
"  <xfoo:defineme inherit='SomeParent' variable='3' >\n"
"    <hbox spacing='3' >\n"
"      <hpaned height='120' position='240' >\n"
"	  <hscrollbar id='need-a-handle' size:vgroup='vg-size' />\n"
"      </hpaned>\n"
"      <vbox spacing='3' >\n"
"	<alignment size:vgroup='vg-size' />\n"
"	<vscrollbar id='another-handle' size:vgroup='vg-size' pack:expand='1' />\n"
"	<alignment size:vgroup='vg-size' />\n"
"      </vbox>\n"
"    </hbox>\n"
"    <emptyzonk></emptyzonk>\n"
"    <emptyline>\n"
"    </emptyline>\n"
"  </xfoo:defineme>\n"
"  \n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"  <nest>\n"
"    <nest>\n"
"      <nest>\n"
"        <nest>\n"
"          <nest>\n"
"            <nest>\n"
"              <nest>\n"
"                <nest>\n"
"                  <nest>\n"
"                    <nest>\n"
"                      <nest>\n"
"                        <nest>\n"
"                          <nest>\n"
"                            <nest>\n"
"                              <nest>\n"
"                              </nest>\n"
"                            </nest>\n"
"                          </nest>\n"
"                        </nest>\n"
"                      </nest>\n"
"                    </nest>\n"
"                  </nest>\n"
"                  <nest>\n"
"                    <nest>\n"
"                      <nest>\n"
"                        <nest>\n"
"                          <nest>\n"
"                            <nest>\n"
"                              <nest>\n"
"                              </nest>\n"
"                            </nest>\n"
"                          </nest>\n"
"                        </nest>\n"
"                      </nest>\n"
"                    </nest>\n"
"                  </nest>\n"
"                </nest>\n"
"              </nest>\n"
"            </nest>\n"
"          </nest>\n"
"        </nest>\n"
"      </nest>\n"
"    </nest>\n"
"  </nest>\n"
"  <!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!----><!---->\n"
"\n"
"  \n"
"  \n"
"</toplevel-tag>\n";

} // anon
