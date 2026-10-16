#include "cases.h"
#include "shanshui/arch.h"

using namespace ss;

SS_CASE("arch01_a1", "a1") {
  return arch01(100, 300, 0.5).svg();
}

SS_CASE("arch01m_a1", "a1") {
  ArchArgs a;
  a.wid = 187.3;
  a.hei = 91.2;
  a.per = 0.37;
  return arch01(420.5, 380, 0.731, a).svg();
}

SS_CASE("arch02_a1", "a1") {
  return arch02(100, 300, 2).svg();
}

SS_CASE("arch02s1_a1", "a1") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 1;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s2_a1", "a1") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 2;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s3_a1", "a1") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 3;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02lo_a1", "a1") {
  ArchArgs a;
  a.wid = 44.1;
  a.sto = 2;
  a.rot = 0.21;
  a.sty = 3;
  a.rai = true;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02one_a1", "a1") {
  ArchArgs a;
  a.wid = 61.5;
  a.sto = 1;
  a.rot = 0.4;
  a.sty = 2;
  return arch02(300, 400, 1, a).svg();
}

SS_CASE("arch03_a1", "a1") {
  return arch03(100, 300, 3).svg();
}

SS_CASE("arch03m_a1", "a1") {
  ArchArgs a;
  a.sto = 5;
  a.wid = 53.81;
  return arch03(612.7, 250.3, 3, a).svg();
}

SS_CASE("arch04_a1", "a1") {
  return arch04(100, 300, 3).svg();
}

SS_CASE("arch04m_a1", "a1") {
  ArchArgs a;
  a.sto = 1;
  a.rot = 0.3;
  return arch04(333.3, 444.4, 3, a).svg();
}

SS_CASE("boat_a1", "a1") {
  return boat01(200, 500, 0.3).svg();
}

SS_CASE("boatfli_a1", "a1") {
  ArchArgs a;
  a.sca = 433.7 / 800;
  a.fli = true;
  return boat01(812.4, 433.7, 0.3, a).svg();
}

SS_CASE("boatnf_a1", "a1") {
  ArchArgs a;
  a.sca = 633.7 / 800;
  a.fli = false;
  return boat01(812.4, 633.7, 0.3, a).svg();
}

SS_CASE("tower_a1", "a1") {
  return transmissionTower01(150.5, 320.25, 4).svg();
}

SS_CASE("man_a1", "a1") {
  return man(100, 300).svg();
}

SS_CASE("manb_a1", "a1") {
  ManArgs a;
  a.fli = false;
  a.sca = 0.42;
  return man(123.4, 345.6, a).svg();
}

SS_CASE("manc_a1", "a1") {
  ManArgs a;
  a.sca = 0.8;
  a.hat = hat02;
  a.ite = stick01;
  a.len = {0, 30, 20, 30, 10, 30, 30, 30, 30};
  return man(50, 60, a).svg();
}

SS_CASE("arch01_b2", "b2") {
  return arch01(100, 300, 0.5).svg();
}

SS_CASE("arch01m_b2", "b2") {
  ArchArgs a;
  a.wid = 187.3;
  a.hei = 91.2;
  a.per = 0.37;
  return arch01(420.5, 380, 0.731, a).svg();
}

SS_CASE("arch02_b2", "b2") {
  return arch02(100, 300, 2).svg();
}

SS_CASE("arch02s1_b2", "b2") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 1;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s2_b2", "b2") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 2;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s3_b2", "b2") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 3;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02lo_b2", "b2") {
  ArchArgs a;
  a.wid = 44.1;
  a.sto = 2;
  a.rot = 0.21;
  a.sty = 3;
  a.rai = true;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02one_b2", "b2") {
  ArchArgs a;
  a.wid = 61.5;
  a.sto = 1;
  a.rot = 0.4;
  a.sty = 2;
  return arch02(300, 400, 1, a).svg();
}

SS_CASE("arch03_b2", "b2") {
  return arch03(100, 300, 3).svg();
}

SS_CASE("arch03m_b2", "b2") {
  ArchArgs a;
  a.sto = 5;
  a.wid = 53.81;
  return arch03(612.7, 250.3, 3, a).svg();
}

SS_CASE("arch04_b2", "b2") {
  return arch04(100, 300, 3).svg();
}

SS_CASE("arch04m_b2", "b2") {
  ArchArgs a;
  a.sto = 1;
  a.rot = 0.3;
  return arch04(333.3, 444.4, 3, a).svg();
}

SS_CASE("boat_b2", "b2") {
  return boat01(200, 500, 0.3).svg();
}

SS_CASE("boatfli_b2", "b2") {
  ArchArgs a;
  a.sca = 433.7 / 800;
  a.fli = true;
  return boat01(812.4, 433.7, 0.3, a).svg();
}

SS_CASE("boatnf_b2", "b2") {
  ArchArgs a;
  a.sca = 633.7 / 800;
  a.fli = false;
  return boat01(812.4, 633.7, 0.3, a).svg();
}

SS_CASE("tower_b2", "b2") {
  return transmissionTower01(150.5, 320.25, 4).svg();
}

SS_CASE("man_b2", "b2") {
  return man(100, 300).svg();
}

SS_CASE("manb_b2", "b2") {
  ManArgs a;
  a.fli = false;
  a.sca = 0.42;
  return man(123.4, 345.6, a).svg();
}

SS_CASE("manc_b2", "b2") {
  ManArgs a;
  a.sca = 0.8;
  a.hat = hat02;
  a.ite = stick01;
  a.len = {0, 30, 20, 30, 10, 30, 30, 30, 30};
  return man(50, 60, a).svg();
}

SS_CASE("arch01_c3", "c3") {
  return arch01(100, 300, 0.5).svg();
}

SS_CASE("arch01m_c3", "c3") {
  ArchArgs a;
  a.wid = 187.3;
  a.hei = 91.2;
  a.per = 0.37;
  return arch01(420.5, 380, 0.731, a).svg();
}

SS_CASE("arch02_c3", "c3") {
  return arch02(100, 300, 2).svg();
}

SS_CASE("arch02s1_c3", "c3") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 1;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s2_c3", "c3") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 2;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02s3_c3", "c3") {
  ArchArgs a;
  a.wid = 57.3;
  a.sto = 3;
  a.rot = 0.62;
  a.sty = 3;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02lo_c3", "c3") {
  ArchArgs a;
  a.wid = 44.1;
  a.sto = 2;
  a.rot = 0.21;
  a.sty = 3;
  a.rai = true;
  return arch02(250.25, 410.5, 7, a).svg();
}

SS_CASE("arch02one_c3", "c3") {
  ArchArgs a;
  a.wid = 61.5;
  a.sto = 1;
  a.rot = 0.4;
  a.sty = 2;
  return arch02(300, 400, 1, a).svg();
}

SS_CASE("arch03_c3", "c3") {
  return arch03(100, 300, 3).svg();
}

SS_CASE("arch03m_c3", "c3") {
  ArchArgs a;
  a.sto = 5;
  a.wid = 53.81;
  return arch03(612.7, 250.3, 3, a).svg();
}

SS_CASE("arch04_c3", "c3") {
  return arch04(100, 300, 3).svg();
}

SS_CASE("arch04m_c3", "c3") {
  ArchArgs a;
  a.sto = 1;
  a.rot = 0.3;
  return arch04(333.3, 444.4, 3, a).svg();
}

SS_CASE("boat_c3", "c3") {
  return boat01(200, 500, 0.3).svg();
}

SS_CASE("boatfli_c3", "c3") {
  ArchArgs a;
  a.sca = 433.7 / 800;
  a.fli = true;
  return boat01(812.4, 433.7, 0.3, a).svg();
}

SS_CASE("boatnf_c3", "c3") {
  ArchArgs a;
  a.sca = 633.7 / 800;
  a.fli = false;
  return boat01(812.4, 633.7, 0.3, a).svg();
}

SS_CASE("tower_c3", "c3") {
  return transmissionTower01(150.5, 320.25, 4).svg();
}

SS_CASE("man_c3", "c3") {
  return man(100, 300).svg();
}

SS_CASE("manb_c3", "c3") {
  ManArgs a;
  a.fli = false;
  a.sca = 0.42;
  return man(123.4, 345.6, a).svg();
}

SS_CASE("manc_c3", "c3") {
  ManArgs a;
  a.sca = 0.8;
  a.hat = hat02;
  a.ite = stick01;
  a.len = {0, 30, 20, 30, 10, 30, 30, 30, 30};
  return man(50, 60, a).svg();
}
