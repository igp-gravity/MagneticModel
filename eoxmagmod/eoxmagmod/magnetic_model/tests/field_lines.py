#-------------------------------------------------------------------------------
#
#  Field line tracing tests
#
# Author: Martin Paces <martin.paces@eox.at>
#
#-------------------------------------------------------------------------------
# Copyright (C) 2018 EOX IT Services GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies of this Software or works derived from this Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#-------------------------------------------------------------------------------

from unittest import TestCase, main
from numpy import array
from numpy.testing import assert_allclose
from eoxmagmod import (
    decimal_year_to_mjd2000,
    GEODETIC_ABOVE_WGS84, GEOCENTRIC_SPHERICAL, GEOCENTRIC_CARTESIAN, convert
)
from eoxmagmod.data import IGRF12
from eoxmagmod.magnetic_model.loader_shc import load_model_shc
from eoxmagmod.magnetic_model.field_lines import trace_field_line


START_POINT = (45, 30, 400)
FIELD_LINE_POINTS = [
    (-35.368274996757364, 39.700301184417704, -95.91876095112734),
    (-35.01448795969475, 39.38936383790091, -8.343611242758463),
    (-34.65414363332517, 39.09143885427224, 80.46632581641443),
    (-34.287149154580035, 38.80570910775888, 170.48385743954177),
    (-33.91340521112774, 38.53140370245006, 261.6822730851895),
    (-33.53280735294822, 38.267798475813166, 354.0351700406412),
    (-33.1452470118385, 38.01421548215414, 447.51625673497267),
    (-32.750612275176636, 37.7700217364484, 542.0991419682955),
    (-32.348788456873976, 37.53462744515785, 637.7571157694631),
    (-31.939658503966836, 37.30748390300443, 734.4629256736445),
    (-31.52310327245107, 37.08808119402684, 832.188550741245),
    (-31.09900170120541, 36.87594580155175, 930.9049745552805),
    (-30.667230908453966, 36.67063820441013, 1030.5819576474994),
    (-30.227666231323482, 36.471750515034486, 1131.1878092395004),
    (-29.78018122571178, 36.27890419812684, 1232.689157814918),
    (-29.324647640898338, 36.091747895561944, 1335.0507197839638),
    (-28.860935381065538, 35.909955373337105, 1438.2350653505048),
    (-28.38891246411117, 35.73322359905164, 1542.2023806207974),
    (-27.90844498676852, 35.561270953051796, 1646.9102249578998),
    (-27.41939710405605, 35.39383557256511, 1752.3132826146998),
    (-26.92163103040731, 35.230673825512135, 1858.3631077202954),
    (-26.415007069440566, 35.07155890893757, 1965.007861777713),
    (-25.89938367917968, 34.91627956592198, 2072.192042940848),
    (-25.37461757960237, 34.764638914245836, 2179.856206466061),
    (-24.84056390964275, 34.61645337984352, 2287.936675905992),
    (-24.29707644118915, 34.47155172810397, 2396.36524481469),
    (-23.744007858176246, 34.32977418626811, 2505.0688689659514),
    (-23.181210109552595, 34.19097165048101, 2613.969349395268),
    (-22.60853484569234, 34.055004971435714, 2722.9830069089776),
    (-22.0258339486906, 33.92174431296016, 2832.0203491323055),
    (-21.43296016791004, 33.7910685783249, 2940.9857316681073),
    (-20.829767873099758, 33.66286489946678, 3049.777015521307),
    (-20.21611393834443, 33.537028184717805, 3158.2852236533854),
    (-19.5918587709697, 33.41346072098646, 3266.394200342208),
    (-18.956867500260778, 33.29207182665039, 3373.980277975981),
    (-18.311011341359094, 33.1727775516753, 3480.9119570125376),
    (-17.65416914988038, 33.05550042166596, 3587.0496060803525),
    (-16.986229182513465, 32.94016922267288, 3692.2451906135816),
    (-16.307091077955235, 32.82671882361354, 3796.342039979603),
    (-15.616668070825733, 32.715090033111295, 3899.174664759246),
    (-14.914889448473955, 32.605229487400294, 4000.5686376735853),
    (-14.201703256592454, 32.49708956568428, 4100.340553530705),
    (-13.477079254053594, 32.39062832896536, 4198.298085460549),
    (-12.74101211011022, 32.28580947787746, 4294.2401564873835),
    (-11.993524827836492, 32.182602324469904, 4387.957247037305),
    (-11.234672366244187, 32.08098177220725, 4479.2318601212155),
    (-10.464545419812909, 31.980928297701848, 4567.839166459296),
    (-9.683274298285431, 31.882427926917643, 4653.5478514627275),
    (-8.891032831773657, 31.785472197828597, 4736.121184517072),
    (-8.088042207038903, 31.69005810085546, 4815.318328103372),
    (-7.274574621123668, 31.596187987929785, 4890.895899676693),
    (-6.450956619559903, 31.5038694408466, 4962.609792711119),
    (-5.617571969778859, 31.41311508978187, 5030.217254715954),
    (-4.774863908051139, 31.323942373578678, 5093.479209429139),
    (-3.9233365924680697, 31.236373234746175, 5152.162797945735),
    (-3.063555597347234, 31.150433744133263, 5206.044099754603),
    (-2.1961472979805383, 31.06615365295016, 5254.910980237076),
    (-1.321797020253233, 30.983565873161062, 5298.565997160113),
    (-0.44124586783960634, 30.902705891123393, 5336.829286243155),
    (0.4447138103142732, 30.823611123479452, 5369.5413363602565),
    (1.3352442898544556, 30.7463202284137, 5396.565559641328),
    (2.2294695901886556, 30.67087238911697, 5417.790561738033),
    (3.126483688099488, 30.597306589277835, 5433.132023508345),
    (4.025359608621255, 30.525660902311937, 5442.534117529116),
    (4.925159123944251, 30.455971816588935, 5445.970400588792),
    (5.824942754416151, 30.388273617996692, 5443.44414558897),
    (6.723779748080034, 30.322597848819587, 5434.988101322176),
    (7.620757718372649, 30.258972858287184, 5420.663694434347),
    (8.514991643138769, 30.19742345558996, 5400.559712377128),
    (9.405631969406274, 30.13797067107105, 5374.790527381686),
    (10.29187162310827, 30.08063162613404, 5343.493937973663),
    (11.172951785773765, 30.025419507579517, 5306.828715291518),
    (12.048166365473644, 29.972343637946285, 5264.971946305724),
    (12.916865151755497, 29.921409630229302, 5218.11626510594),
    (13.778455699650472, 29.87261961319553, 5166.4670576997105),
    (14.6324040331183, 29.82597251242545, 5110.239716210898),
    (15.478234291947636, 29.781464372080617, 5049.657006330503),
    (16.315527467884976, 29.73908870307601, 4984.946598543721),
    (17.143919386417263, 29.698836844625532, 4916.338800153438),
    (17.963098091708947, 29.660698327823923, 4844.064512303638),
    (18.77280078563276, 29.624661231839266, 4768.353424718179),
    (19.572810459698225, 29.59071252525364, 4689.4324510854285),
    (20.362952342940826, 29.55883838697974, 4607.524400112143),
    (21.143090271212472, 29.529024502908456, 4522.846871214492),
    (21.913123065211792, 29.50125633595237, 4435.6113595192655),
    (22.67298098707163, 29.475519368416595, 4346.022552066801),
    (23.422622329111146, 29.451799316649225, 4254.2777956161535),
    (24.1620301739114, 29.430082318710383, 4160.5667160045305),
    (24.891209352402193, 29.410355096376488, 4065.0709693314634),
    (25.610183616198505, 29.3926050931917, 3967.9641061440957),
    (26.31899303191521, 29.376820590523245, 3869.411531086794),
    (27.0176915984609, 29.362990803700338, 3769.570541990718),
    (27.70634508315988, 29.351105960345272, 3668.5904340019447),
    (28.385029068755717, 29.341157362963145, 3566.6126560088856),
    (29.05382720068859, 29.333137437764957, 3463.771008224052),
    (29.712829622304703, 29.327039771572494, 3360.191871297074),
    (30.362131584662148, 29.322859138506708, 3255.9944587413106),
    (31.001832217179633, 29.320591518003912, 3151.291085728526),
    (31.632033445393088, 29.320234105543264, 3046.1874484433674),
    (32.25283904242543, 29.32178531731026, 2940.7829091901353),
    (32.86435380134023, 29.325244789867906, 2835.1707833303),
    (33.46668281626752, 29.3306133757618, 2729.438624871505),
    (34.05993086099915, 29.33789313584865, 2623.6685081916003),
    (34.644201854606706, 29.34708732901021, 2517.9373039172647),
    (35.2195984045041, 29.358200399795482, 2412.31694745271),
    (35.7862214182336, 29.37123796442264, 2306.874699029434),
    (36.3441697760819, 29.38620679546709, 2201.673394479185),
    (36.89354005741947, 29.403114805461755, 2096.771686189296),
    (37.4344263143956, 29.421971029538316, 1992.224273917965),
    (37.9669198873078, 29.442785607141523, 1888.0821253214035),
    (38.491109256598534, 29.465569762750437, 1784.392686181506),
    (39.00707992701201, 29.490335785437964, 1681.2000804353756),
    (39.514914339973785, 29.51709700699035, 1578.5453001896585),
    (40.01469181073535, 29.54586777818758, 1476.4663859706093),
    (40.506488487260484, 29.576663442710245, 1374.9985975068655),
    (40.99037732822065, 29.609500307983723, 1274.174575373318),
    (41.46642809781705, 29.64439561209029, 1174.024493854608),
    (41.93470737546064, 29.681367485668204, 1074.5762053888166),
    (42.39527857861846, 29.720434907465563, 975.8553769697422),
    (42.848201997380436, 29.761617651915476, 877.8856188725068),
    (43.29353483951309, 29.804936226737404, 780.6886060763069),
    (43.7313312849494, 29.850411798131685, 684.2841927326435),
    (44.16164254881369, 29.898066100603295, 588.6905200287431),
    (44.584516952199515, 29.947921327804714, 493.92411777134504),
    (45.0, 30.0, 400.0),
    (45.4271000089397, 30.054327754522603, 306.4446284174588),
    (45.84643903260964, 30.111032625022695, 213.71898027239706),
    (46.258063637179326, 30.170141590417526, 121.83824229187483),
    (46.6620177206953, 30.231681544777825, 30.816138423313017),
    (47.058342603855564, 30.29567899837887, -59.33498391874138),
]


class TestFieldLineTracing(TestCase):
    time = decimal_year_to_mjd2000(2015.5)
    point = START_POINT
    points = FIELD_LINE_POINTS

    @property
    def model(self):
        if not hasattr(self, "_model"):
            self._model = load_model_shc(IGRF12)
        return self._model

    def test_trace_field_line_wgs84(self):
        point = self.point
        points, vectors = trace_field_line(
            self.model, self.time, point,
            input_coordinate_system=GEODETIC_ABOVE_WGS84,
            output_coordinate_system=GEODETIC_ABOVE_WGS84,
        )
        assert_allclose(points, self.points)
        assert_allclose(vectors, self.model.eval(
            self.time, points,
            input_coordinate_system=GEODETIC_ABOVE_WGS84,
            output_coordinate_system=GEODETIC_ABOVE_WGS84,
        ))

    def test_trace_field_line_spherical(self):
        point = convert(self.point, GEODETIC_ABOVE_WGS84, GEOCENTRIC_SPHERICAL)
        points, vectors = trace_field_line(
            self.model, self.time, point,
            input_coordinate_system=GEOCENTRIC_SPHERICAL,
            output_coordinate_system=GEOCENTRIC_SPHERICAL,
        )
        assert_allclose(points, convert(
            self.points, GEODETIC_ABOVE_WGS84, GEOCENTRIC_SPHERICAL
        ))
        assert_allclose(vectors, self.model.eval(
            self.time, points,
            input_coordinate_system=GEOCENTRIC_SPHERICAL,
            output_coordinate_system=GEOCENTRIC_SPHERICAL,
        ))

    def test_trace_field_line_cartesian(self):
        point = convert(self.point, GEODETIC_ABOVE_WGS84, GEOCENTRIC_CARTESIAN)
        points, vectors = trace_field_line(
            self.model, self.time, point,
            input_coordinate_system=GEOCENTRIC_CARTESIAN,
            output_coordinate_system=GEOCENTRIC_CARTESIAN,
        )
        assert_allclose(points, convert(
            self.points, GEODETIC_ABOVE_WGS84, GEOCENTRIC_CARTESIAN
        ))
        assert_allclose(vectors, self.model.eval(
            self.time, points,
            input_coordinate_system=GEOCENTRIC_CARTESIAN,
            output_coordinate_system=GEOCENTRIC_CARTESIAN,
        ))


if __name__ == "__main__":
    main()
