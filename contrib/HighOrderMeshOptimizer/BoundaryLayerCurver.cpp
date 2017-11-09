// Copyright (C) 2017 ULg-UCL
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, and/or sell copies of the
// Software, and to permit persons to whom the Software is furnished
// to do so, provided that the above copyright notice(s) and this
// permission notice appear in all copies of the Software and that
// both the above copyright notice(s) and this permission notice
// appear in supporting documentation.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR
// ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
// ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
// Please report all bugs and problems to the public mailing list
// <gmsh@onelab.info>.
//
// Contributors: Amaury Johnen

#include "BoundaryLayerCurver.h"
#include "MQuadrangle.h"
#include "MTriangle.h"
#include "BasisFactory.h"
#include "GFace.h"
#include "legendrePolynomials.h"
#include "bezierBasis.h"

#include "gmshVertex.h"
#include "Geo.h"
#include "MLine.h"

//#include "MElement.h"
//#include "MLine.h"
//#include "GmshDefines.h"

namespace
{
  void drawAmplifiedDiffWithLin(const std::vector<MVertex *> &vertices,
                                GEntity *entity, double amp = 100,
                                bool drawBasisLines = false, int cnt = 100)
  {
    MLineN l(vertices);
    MVertex *v0 = vertices[0];
    MVertex *v1 = vertices[1];
    MVertex *previous = NULL;
    for (int i = 0; i < cnt; ++i) {
      double xi = -1 + 2 * i / (cnt - 1.);
      double c1 = i / (cnt - 1.);
      double c0 = 1 - c1;
      SPoint3 pLin(v0->x() * c0 + v1->x() * c1,
                   v0->y() * c0 + v1->y() * c1,
                   v0->z() * c0 + v1->z() * c1);
      SPoint3 p;
      l.pnt(xi, 0, 0, p);
      MVertex *vLin = new MVertex(pLin.x(), pLin.y(), pLin.z(), entity);
      MVertex *v = new MVertex(amp * p.x() - (amp - 1) * pLin.x(),
                               amp * p.y() - (amp - 1) * pLin.y(),
                               amp * p.z() - (amp - 1) * pLin.z(), entity);
      if (previous) {
        MLine *line = new MLine(v, previous);
        ((GEdge *) entity)->addLine(line);
      }
      if (drawBasisLines) {
        MLine *line = new MLine(v, vLin);
        ((GEdge *) entity)->addLine(line);
        ((GEdge *) entity)->addMeshVertex(vLin);
      }
      ((GEdge *) entity)->addMeshVertex(v);
      previous = v;
    }
  }

  void drawBezierControlPolygon(const std::vector<MVertex *> &vertices,
                                GEntity *entity)
  {
    const int nVert = vertices.size();
    const bezierBasis *fs = BasisFactory::getBezierBasis(TYPE_LIN, nVert - 1);

    fullMatrix<double> xyz(nVert, 3);
    for (int i = 0; i < nVert; ++i) {
      xyz(i, 0) = vertices[i]->x();
      xyz(i, 1) = vertices[i]->y();
      xyz(i, 2) = vertices[i]->z();
    }
    fullMatrix<double> controlPoints(nVert, 3);
    fs->lag2Bez(xyz, controlPoints);

    std::vector<int> idx(nVert);
    idx[0] = 0;
    for (int i = 1; i < nVert-1; ++i) idx[i] = i+1;
    idx[nVert-1] = 1;

    MVertex *previous = NULL;
    for (int i = 0; i < nVert; ++i) {
      MVertex *v = new MVertex(controlPoints(idx[i], 0), controlPoints(idx[i], 1),
                               controlPoints(idx[i], 2), entity);
      if (previous) {
        MLine *line = new MLine(v, previous);
        ((GEdge *) entity)->addLine(line);
      }
      ((GEdge *) entity)->addMeshVertex(v);
      previous = v;
    }
  }

  void drawBezierDerivative(const std::vector<MVertex *> &vertices,
                            GEntity *entity, SPoint3 p, int *onlyN = NULL,
                            double fact = 1)
  {
    MVertex *v = new MVertex(p.x(), p.y(), p.z(), entity);
    MLine *line = new MLine(v, v);
    ((GEdge *) entity)->addLine(line);
    ((GEdge *) entity)->addMeshVertex(v);


    const int nVert = vertices.size();

    fullMatrix<double> controlPoints(nVert, 3);
    {
      const bezierBasis *fs = BasisFactory::getBezierBasis(TYPE_LIN, nVert - 1);
      fullMatrix<double> xyz(nVert, 3);
      for (int i = 0; i < nVert; ++i) {
        xyz(i, 0) = vertices[i]->x();
        xyz(i, 1) = vertices[i]->y();
        xyz(i, 2) = vertices[i]->z();
      }
      fs->lag2Bez(xyz, controlPoints);
    }

    fullMatrix<double> deriv(nVert, 3);
    deriv(0, 0) = controlPoints(0, 0);
    deriv(0, 1) = controlPoints(0, 1);
    deriv(0, 2) = controlPoints(0, 2);
    for (int i = 1; i < nVert-1; ++i) {
      deriv(i, 0) = controlPoints(i+1, 0);
      deriv(i, 1) = controlPoints(i+1, 1);
      deriv(i, 2) = controlPoints(i+1, 2);
    }
    deriv(nVert-1, 0) = controlPoints(1, 0);
    deriv(nVert-1, 1) = controlPoints(1, 1);
    deriv(nVert-1, 2) = controlPoints(1, 2);
    deriv.scale(fact);

    for (int n = nVert - 1; n > 0; --n) {
      for (int i = 0; i < n; ++i) {
        deriv(i, 0) = deriv(i+1, 0) - deriv(i, 0);
        deriv(i, 1) = deriv(i+1, 1) - deriv(i, 1);
        deriv(i, 2) = deriv(i+1, 2) - deriv(i, 2);
      }
      deriv.scale(n);
      if (!onlyN || *onlyN == n) {
        MVertex *previous = NULL;
        for (int i = 0; i < n; ++i) {
          MVertex *v = new MVertex(p.x() + deriv(i, 0), p.y() + deriv(i, 1),
                                   p.z() + deriv(i, 2), entity);
          if (previous) {
            MLine *line = new MLine(v, previous);
            ((GEdge *) entity)->addLine(line);
            ((GEdge *) entity)->addMeshVertex(v);
          }
          else if (n == 1)
            ((GEdge *) entity)->addMeshVertex(v);
          previous = v;
        }
      }
    }
  }

  void drawIdealCurve(const std::vector<MVertex *> &baseVert,
                      BoundaryLayerCurver::Parameters2DCurve &parameters, 
                      SVector3 w, GEntity *entity, bool drawh = true,
                      bool drawn = false, bool drawt = false, int cnt = 100)
  {
    int tagLine = ElementType::getTag(TYPE_LIN, baseVert.size() - 1);
    const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);

    MVertex *vh0 = NULL;
    MVertex *vn0 = NULL;
    MVertex *vt0 = NULL;

    for (int i = 0; i < cnt; ++i) {
      double xi = -1 + 2 * i / (cnt - 1.);
      double xc = 0, yc = 0, zc = 0;
      double dx = 0, dy = 0, dz = 0;
      {
        double f[100];
        double sf[100][3];
        fs->f(xi, 0, 0, f);
        fs->df(xi, 0, 0, sf);
        for (int j = 0; j < fs->getNumShapeFunctions(); j++) {
          const MVertex *v = baseVert[j];
          xc += f[j] * v->x();
          yc += f[j] * v->y();
          zc += f[j] * v->z();
          dx += sf[j][0] * v->x();
          dy += sf[j][0] * v->y();
          dz += sf[j][0] * v->z();
        }
      }
      SVector3 t, n, h;
      double x, y, z;
      MVertex *v;
      MLine *line;
      t = SVector3(dx, dy, dz).unit();
      n = crossprod(w, t);

      if (drawh) {
        h = parameters.thicknessAtPoint(xi) * n +
            parameters.coeffbAtPoint(xi) * t;
        x = xc + h.x();
        y = yc + h.y();
        z = zc + h.z();
        v = new MVertex(x, y, z, entity);
        if (vh0) {
          line = new MLine(v, vh0);
          ((GEdge *) entity)->addLine(line);
        }
        ((GEdge *) entity)->addMeshVertex(v);
        vh0 = v;
      }

      if (drawn) {
        h = parameters.thicknessAtPoint(xi) * n;
        x = xc + h.x();
        y = yc + h.y();
        z = zc + h.z();
        v = new MVertex(x, y, z, entity);
        if (vn0) {
          line = new MLine(v, vn0);
          ((GEdge *) entity)->addLine(line);
        }
        ((GEdge *) entity)->addMeshVertex(v);
        vn0 = v;
      }

      if (drawt) {
        h = parameters.coeffbAtPoint(xi) * t;
        x = xc + h.x();
        y = yc + h.y();
        z = zc + h.z();
        v = new MVertex(x, y, z, entity);
        if (vt0) {
          line = new MLine(v, vt0);
          ((GEdge *) entity)->addLine(line);
        }
        ((GEdge *) entity)->addMeshVertex(v);
        vt0 = v;
      }
    }
  }
}

namespace BoundaryLayerCurver
{
  typedef std::pair<int, std::pair<int, int> > TupleLeastSquareData;
  static std::map<TupleLeastSquareData, LeastSquareData*> leastSquareData;
  static std::map<int, InteriorPlacementData*> interiorPlacementData;

  LeastSquareData* constructLeastSquareData(int typeElement, int order,
                                            int orderGauss)
  {
    // invM1 gives
    //     value of coefficients in Legendre basis
    //   + value of Lagrange multipliers
    // from
    //     Ij + value of function f at extremities
    // M2 gives
    //     Ij = integral of product function f with Legendre polynomial j
    //   + value of function f at extremities
    // from
    //     the values of function f at Gauss points
    //   + value of function f at extremities
    LeastSquareData *data = new LeastSquareData;
    if (typeElement == TYPE_LIN) {
      data->nbPoints = getNGQLPts(orderGauss);
      data->intPoints = getGQLPts(orderGauss);
      LegendrePolynomials legendre(order);

      int sz1 = order + 1;
      int sz2 = data->nbPoints;
      fullMatrix<double> M2(sz1+2, sz2+2, true);
      {
        double *val = new double[sz1];
        for (int j = 0; j < sz2; ++j) {
          legendre.f(data->intPoints[j].pt[0], val);
          for (int i = 0; i < sz1; ++i) {
            M2(i, j) = 2 * val[i] * data->intPoints[j].weight;
          }
        }
        M2(sz1, sz2) = M2(sz1+1, sz2+1) = 1;
        delete val;
      }

      fullMatrix<double> M1(sz1+2, sz1+2, true);
      for (int k = 0; k < sz1; ++k) {
        const int sign = k % 2 == 0 ? 1 : -1;
        M1(sz1, k) = M1(k, sz1) = sign;
        M1(sz1+1, k) = M1(k, sz1+1) = 1;
        M1(k, k) = 4. / (1 + 2*k);
      }
      fullMatrix<double> invM1;
      M1.invert(invM1);

      fullMatrix<double> Leg2Leg(sz1+2, sz1+2, true);
      {
        fullMatrix<double> MM1(sz1+2, sz1+2, true);
        fullMatrix<double> &MM2 = Leg2Leg;
        int tagLine = ElementType::getTag(TYPE_LIN, order);
        const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);
        const fullMatrix<double> &refNodes = fs->getReferenceNodes();
        double *val = new double[sz1];
        double *valc = new double[sz1];
        for (int i = 0; i < sz1; ++i) {
          legendre.f(refNodes(i, 0), val);
          legendre.fc(refNodes(i, 0), valc);
          for (int j = 0; j < sz1; ++j) {
            MM1(i, j) = val[j];
            MM2(i, j) = valc[j];
          }
        }
        MM1(sz1, sz1) = MM1(sz1+1, sz1+1) = 1;
        MM2(sz1, sz1) = MM2(sz1+1, sz1+1) = 1;
        fullMatrix<double> invMM2(sz1+2, sz1+2, true);
        MM2.invert(invMM2);
        invMM2.mult(MM1, Leg2Leg);
      }

      data->Leg2Lag.resize(sz1, sz1, true);
      {
        int tagLine = ElementType::getTag(TYPE_LIN, order);
        const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);
        const fullMatrix<double> &refNodes = fs->getReferenceNodes();
        double *val = new double[sz1];
        for (int i = 0; i < sz1; ++i) {
          legendre.fc(refNodes(i, 0), val);
          for (int j = 0; j < sz1; ++j) {
            data->Leg2Lag(i, j) = val[j];
          }
//          data->Leg2Lag.print("data->Leg2Lag");
        }
        //data->Leg2Lag(sz1, sz1) = data->Leg2Lag(sz1+1, sz1+1) = 1;
      }

      /*{
        fullMatrix<double> Leg2Lag(sz1+2, sz1+2, true);
        {
          int tagLine = ElementType::getTag(TYPE_LIN, order);
          const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);
          const fullMatrix<double> &refNodes = fs->getReferenceNodes();
          double *val = new double[sz1];
          for (int i = 0; i < sz1; ++i) {
            legendre.f(refNodes(i, 0), val);
            for (int j = 0; j < sz1; ++j) {
              Leg2Lag(i, j) = val[j];
            }
          }
          Leg2Lag(sz1, sz1) = Leg2Lag(sz1+1, sz1+1) = 1;
        }
        Leg2Lag.print("Leg2Lag");
      }*/

      data->Leg2p.resize(sz2, sz1, true);
      {
        int tagLine = ElementType::getTag(TYPE_LIN, order);
        const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);
        const fullMatrix<double> &refNodes = fs->getReferenceNodes();
        double *val = new double[sz1];
        for (int i = 0; i < sz2; ++i) {
          legendre.fc(data->intPoints[i].pt[0], val);
          for (int j = 0; j < sz1; ++j) {
            data->Leg2p(i, j) = val[j];
          }
//          data->Leg2p.print("data->Leg2Lag");
        }
      }

//      M1.print("M1");
//      invM1.print("invM1");
//      M2.print("M2");
//      Leg2Leg.print("Leg2Leg");
//      data->Leg2Lag.print("data->Leg2Lag");

      fullMatrix<double> &tmp = M1;
      tmp.resize(sz1+2, sz2+2, false);
      invM1.mult(M2, tmp);
//      tmp.print("tmp");

      fullMatrix<double> &tmp2 = M2;
      tmp2.resize(sz1+2, sz2+2, false);
      Leg2Leg.mult(tmp, tmp2);
//      tmp2.print("tmp2");

      data->invA.resize(sz1, sz2+2, false);
      data->invA.copy(tmp2, 0, sz1, 0, sz2+2, 0, 0);
//      data->invA.print("invA");
      return data;
    }
    else if (typeElement == TYPE_QUA) {
      Msg::Error("Implement data for quad");
    }
    else if (typeElement == TYPE_TRI) {
      Msg::Error("Implement data for tri");
    }
  }

  LeastSquareData* getLeastSquareData(int typeElement, int order,
                                      int orderGauss)
  {
    TupleLeastSquareData typeOrder(typeElement, std::make_pair(order, orderGauss));
    std::map<TupleLeastSquareData, LeastSquareData*>::iterator it;
    it = leastSquareData.find(typeOrder);

    if (it != leastSquareData.end()) return it->second;

    LeastSquareData *data = constructLeastSquareData(typeElement, order,
                                                     orderGauss);
    leastSquareData[typeOrder] = data;
    return data;
  }

  void computeExtremityCoefficients(const std::vector<MVertex*> &baseVert,
                                    const std::vector<MVertex*> &topVert,
                                    Parameters2DCurve &parameters,
                                    SVector3 w)
  {
    int tagLine = ElementType::getTag(TYPE_LIN, baseVert.size() - 1);
    const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);
    double sf[100][3];

    SVector3 t, n, h;
    {
      double dx = 0, dy = 0, dz = 0;
      fs->df(-1, 0, 0, sf);
      for (int j = 0; j < fs->getNumShapeFunctions(); j++) {
        const MVertex *v = baseVert[j];
        dx += sf[j][0] * v->x();
        dy += sf[j][0] * v->y();
        dz += sf[j][0] * v->z();
      }

      t = SVector3(dx, dy, dz).unit();
      n = crossprod(w, t);
      h = SVector3(topVert[0]->x() - baseVert[0]->x(),
                   topVert[0]->y() - baseVert[0]->y(),
                   topVert[0]->z() - baseVert[0]->z());
    }
    parameters.thickness[0] = dot(h, n);
    parameters.coeffb[0] = dot(h, t);

    {
      double dx = 0, dy = 0, dz = 0;
      fs->df(1, 0, 0, sf);
      for (int j = 0; j < fs->getNumShapeFunctions(); j++) {
        const MVertex *v = baseVert[j];
        dx += sf[j][0] * v->x();
        dy += sf[j][0] * v->y();
        dz += sf[j][0] * v->z();
      }

      t = SVector3(dx, dy, dz).unit();
      n = crossprod(w, t);
      h = SVector3(topVert[1]->x() - baseVert[1]->x(),
                   topVert[1]->y() - baseVert[1]->y(),
                   topVert[1]->z() - baseVert[1]->z());
    }
    parameters.thickness[1] = dot(h, n);
    parameters.coeffb[1] = dot(h, t);
  }

  void idealPositionEdge(const std::vector<MVertex *> &baseVert,
                         Parameters2DCurve &parameters, SVector3 w,
                         int nbPoints, const IntPt *points,
                         fullMatrix<double> &xyz, GEntity *bndEnt,
                         int triDirection = 0)
  {
    int tagLine = ElementType::getTag(TYPE_LIN, baseVert.size()-1);
    const nodalBasis *fs = BasisFactory::getNodalBasis(tagLine);

//    drawIdealCurve(baseVert, parameters, w, bndEnt);

    for (int i = 0; i < nbPoints; ++i) {
      double xi = points[i].pt[0];
      double xc = 0, yc = 0, zc = 0;
      double dx = 0, dy = 0, dz = 0;
      {
        double f[100];
        double sf[100][3];
        fs->f(xi, 0, 0, f);
        fs->df(xi, 0, 0, sf);
        for (int j = 0; j < fs->getNumShapeFunctions(); j++) {
          const MVertex *v = baseVert[j];
          xc += f[j] * v->x();
          yc += f[j] * v->y();
          zc += f[j] * v->z();
          dx += sf[j][0] * v->x();
          dy += sf[j][0] * v->y();
          dz += sf[j][0] * v->z();
        }
      }
      SVector3 t, n, h, nn, tt;
      t = SVector3(dx, dy, dz).unit();
      n = crossprod(w, t);
      int &d = triDirection;
      h = parameters.thicknessAtPoint(xi, d) * n
          + parameters.coeffbAtPoint(xi, d) * t;
      nn = parameters.thicknessAtPoint(xi, d) * n;
      tt = parameters.coeffbAtPoint(xi, d) * t;
      xyz(i, 0) = xc + h.x();
      xyz(i, 1) = yc + h.y();
      xyz(i, 2) = zc + h.z();
    }
  }

  void damping(std::vector<MVertex *> &vertices, double limit)
  {
    const int nVert = vertices.size();
    const bezierBasis *fs = BasisFactory::getBezierBasis(TYPE_LIN, nVert - 1);

    fullMatrix<double> xyz(nVert, 3);
    for (int i = 0; i < nVert; ++i) {
      xyz(i, 0) = vertices[i]->x();
      xyz(i, 1) = vertices[i]->y();
      xyz(i, 2) = vertices[i]->z();
    }
    fullMatrix<double> controlPoints(nVert, 3);
    fs->lag2Bez(xyz, controlPoints);

    std::vector<int> idx(nVert);
    idx[0] = 0;
    for (int i = 1; i < nVert-1; ++i) idx[i] = i+1;
    idx[nVert-1] = 1;

    fullMatrix<double> dCtrlPnts(nVert, 3);
    dCtrlPnts(idx[0], 0) = 0;
    dCtrlPnts(idx[0], 1) = 0;
    dCtrlPnts(idx[0], 2) = 0;
    dCtrlPnts(idx[nVert-1], 0) = 0;
    dCtrlPnts(idx[nVert-1], 1) = 0;
    dCtrlPnts(idx[nVert-1], 2) = 0;
    for (int i = 1; i < nVert-1; ++i) {
      for (int j = 0; j < 3; ++j)
        dCtrlPnts(idx[i], j) = .25 * controlPoints(idx[i-1], j) +
                               .25 * controlPoints(idx[i+1], j) -
                               .5 * controlPoints(idx[i], j);
    }

    double maxDisplacement = 0;
    for (int i = 0; i < nVert; ++i) {
      double displacement = std::sqrt(dCtrlPnts(i, 0) * dCtrlPnts(i, 0) +
                                      dCtrlPnts(i, 1) * dCtrlPnts(i, 1) +
                                      dCtrlPnts(i, 2) * dCtrlPnts(i, 2));
      maxDisplacement = std::max(maxDisplacement, displacement);
    }

    if (maxDisplacement > limit)
      dCtrlPnts.scale(limit / maxDisplacement);
    // TODO go to linear in other case?? Don't think is good idea

    controlPoints.add(dCtrlPnts);
    fs->matrixBez2Lag.mult(controlPoints, xyz);
    for (int i = 0; i < nVert; ++i) {
      vertices[i]->x() = xyz(i, 0);
      vertices[i]->y() = xyz(i, 1);
      vertices[i]->z() = xyz(i, 2);
    }
  }

  void computePositionEdgeVert(const std::vector<MVertex *> &baseVert,
                               std::vector<MVertex *> &topVert,
                               Parameters2DCurve &parameters, SVector3 w,
                               double factorDamping, GEntity *bndEnt,
                               int nLayer, bool last = false, int direction = 0)
  {
    // Let (t, n, e3) be the local reference frame
    // We seek for each component the polynomial function that fit the best
    //   x1(xi) = x0(xi) + h(xi) * n(xi) + b(xi) * t(xi)
    // in the least square sense.
    // where x0(xi) is the position of the base edge
    //       n(xi) is the unit normal of the base edge
    //       h(xi) is the linear thickness
    //       t(xi) is the unit tangent of the base edge
    //       b(xi) is the linear coefficient

    if (nLayer == 0) {
//      drawAmplifiedDiffWithLin(baseVert, bndEnt, 20);
//      drawBezierControlPolygon(baseVert, bndEnt);
    }

    const int orderCurve = baseVert.size() - 1;
    const int orderGauss = orderCurve * 2;
    const int sizeSystem = getNGQLPts(orderGauss);
    const IntPt *gaussPnts = getGQLPts(orderGauss);

    fullMatrix<double> xyz(sizeSystem + 2, 3);
    idealPositionEdge(baseVert, parameters, w, sizeSystem, gaussPnts, xyz,
                      bndEnt, direction);
    for (int i = 0; i < 2; ++i) {
      xyz(sizeSystem+i, 0) = topVert[i]->x();
      xyz(sizeSystem+i, 1) = topVert[i]->y();
      xyz(sizeSystem+i, 2) = topVert[i]->z();
    }

    LeastSquareData *data = getLeastSquareData(TYPE_LIN, orderCurve, orderGauss);

    fullMatrix<double> coeff(orderCurve + 1, 3);
    fullMatrix<double> newxyz(orderCurve + 1, 3);
    data->invA.mult(xyz, coeff);
    data->Leg2Lag.mult(coeff, newxyz);

    for (int i = 2; i < topVert.size(); ++i) {
      topVert[i]->x() = newxyz(i, 0);
      topVert[i]->y() = newxyz(i, 1);
      topVert[i]->z() = newxyz(i, 2);
    }

    damping(topVert, factorDamping * parameters.characteristicThickness());

//    if (nLayer == 0)
//      drawBezierControlPolygon(topVert, bndEnt);
//    drawAmplifiedDiffWithLin(topVert, bndEnt, 2);
//    drawBezierControlPolygon(topVert, bndEnt);
    if (last) {
//      drawAmplifiedDiffWithLin(topVert, bndEnt, 2);
//      drawBezierControlPolygon(topVert, bndEnt);
    }
  }

  InteriorPlacementData* constructInteriorPlacementData(int tag)
  {
    const int order = ElementType::OrderFromTag(tag);
    const nodalBasis *fs = BasisFactory::getNodalBasis(tag);
    InteriorPlacementData *data = new InteriorPlacementData;

    int type = ElementType::ParentTypeFromTag(tag);

    switch (type) {
      case TYPE_TRI:
      case TYPE_QUA: {
        std::map<std::pair<int, int>, int> coordinate2num;
        std::vector<std::pair<int, int> > num2coordinate;
        int x, y;
        for (int i = 0; i < fs->points.size1(); ++i) {
          if (type == TYPE_TRI) {
            x = std::rint(fs->points(i, 0) * order);
            y = std::rint(fs->points(i, 1) * order);
          }
          else {
            x = std::rint((fs->points(i, 0) + 1) / 2. * order);
            y = std::rint((fs->points(i, 1) + 1) / 2. * order);
          }
          coordinate2num[std::make_pair(x, y)] = i;
          num2coordinate.push_back(std::make_pair(x, y));
        }

        for (int i = 0; i < fs->points.size1(); ++i) {
          const std::pair<int, int> coordinates(num2coordinate[i]);
          const int &x = coordinates.first;
          const int &y = coordinates.second;
          int y1;
          if (type == TYPE_TRI) {
            y1 = order - x;
            if (y == 0 || y == y1 || x == 0) continue;
          }
          else {
            y1 = order;
            if (y == 0 || y == y1 || x == 0 || x == order) continue;
          }
          const std::pair<int, int> coordinates0(x, 0);
          const std::pair<int, int> coordinates1(x, y1);
          data->iToMove.push_back(i);
          data->factor.push_back(1 - y / (double) y1);
          data->i0.push_back(coordinate2num[coordinates0]);
          data->i1.push_back(coordinate2num[coordinates1]);
        }
        break;
      }
      default:
        Msg::Error("Need to implement constructInteriorPlacementData for other type");
    }

    return data;
  }

  void repositionInteriorNodes(MElement *el)
  {
    int tag = el->getTypeForMSH();
    InteriorPlacementData *data;

    std::map<int, InteriorPlacementData*>::iterator it;
    it = interiorPlacementData.find(tag);
    if (it != interiorPlacementData.end()) data = it->second;
    else {
      data = constructInteriorPlacementData(tag);
      interiorPlacementData[tag] = data;
    }

    for (int i = 0; i < data->iToMove.size(); ++i) {
      MVertex *v = el->getVertex(data->iToMove[i]);
      MVertex *v0 = el->getVertex(data->i0[i]);
      MVertex *v1 = el->getVertex(data->i1[i]);
      v->x() = data->factor[i] * v0->x() + (1 - data->factor[i]) * v1->x();
      v->y() = data->factor[i] * v0->y() + (1 - data->factor[i]) * v1->y();
      v->z() = data->factor[i] * v0->z() + (1 - data->factor[i]) * v1->z();
    }
  }

  MEdge commonEdge(MTriangle *t0, MTriangle *t1)
  {
    MVertex *v[3] = {NULL};
    int k = 0;
    for (int i = 0; i < 3; ++i) {
      if (t0->getVertex(i) == t1->getVertex(0) ||
          t0->getVertex(i) == t1->getVertex(1) ||
          t0->getVertex(i) == t1->getVertex(2)) v[k++] = t0->getVertex(i);
    }
    if (k == 3) Msg::Error("Multiple common edges!");
    if (k > 1) return MEdge(v[0], v[1]);
    Msg::Error("no common edge!!!");
    return MEdge();
  }

  MEdge commonEdge(MTriangle *t, MElement *el)
  {
    MEdge theEdge;
    for (int i = 0; i < el->getNumEdges(); ++i) {
      MEdge e = el->getEdge(i);
      int cntVertInT = 0;
      for (int j = 0; j < 3; ++j) {
        if (e.getVertex(0) == t->getVertex(j) ||
            e.getVertex(1) == t->getVertex(j)) ++cntVertInT;
      }
      if (cntVertInT == 2) {
        if (theEdge.getVertex(0) || theEdge.getVertex(1)) {
          Msg::Error("Multiple common edges or error in default creation!");
        }
        theEdge = e;
      }
    }
    if (theEdge.getVertex(0) && theEdge.getVertex(1)) return theEdge;
    Msg::Error("no common edge!!!");
    return MEdge();
  }

  bool curve2DTriColumn(MElement *bottomEdge, MElement *extElem,
                        std::vector<MElement *> &column,
                        SVector3 w, double dampingFactor, GEntity *bndEnt)
  {
    MEdge bottom(bottomEdge->getVertex(0), bottomEdge->getVertex(1));
    std::vector<MVertex *> bottomVertices, topVertices, midVertices;

    for (int i = 0; i < column.size(); i += 2) {
      MTriangle *tri0 = dynamic_cast<MTriangle *>(column[i]);
      MTriangle *tri1 = dynamic_cast<MTriangle *>(column[i+1]);
      MEdge common = commonEdge(tri0, tri1);
      MEdge top;
      if (i == column.size() - 2)
        top = commonEdge(tri1, extElem);
      else
        top = commonEdge(tri1, dynamic_cast<MTriangle *>(column[i+2]));

      // Reorient if needed
      int iBottom, iTop, sign;
      tri0->getEdgeInfo(bottom, iBottom, sign);
      tri0->getEdgeInfo(common, iTop, sign);
      if (iBottom != 0 && iTop != 0) tri0->reorient(1, false);
      else if (iBottom != 1 && iTop != 1) tri0->reorient(2, false);
      tri1->getEdgeInfo(common, iBottom, sign);
      tri1->getEdgeInfo(top, iTop, sign);
      if (iBottom != 0 && iTop != 0) tri1->reorient(1, false);
      else if (iBottom != 1 && iTop != 1) tri1->reorient(2, false);

      // Get vertices
      tri0->getEdgeInfo(bottom, iBottom, sign);
      tri0->getEdgeVertices(iBottom, bottomVertices);
      tri0->getEdgeVertices(1-iBottom, midVertices);
      std::reverse(midVertices.begin(), midVertices.begin() + 2);
      std::reverse(midVertices.begin() + 2, midVertices.end());

      common = MEdge(midVertices[0], midVertices[1]);
      tri1->getEdgeInfo(common, iBottom, sign);
      tri1->getEdgeVertices(1-iBottom, topVertices);
      if (sign > 0) {
        std::reverse(topVertices.begin(), topVertices.begin() + 2);
        std::reverse(topVertices.begin() + 2, topVertices.end());
      }

      //
      int direction = 1;
      if (bottomVertices[1] == midVertices[1]) direction = -1;
      Parameters2DCurve parameters;
      computeExtremityCoefficients(bottomVertices, topVertices, parameters, w);
      computePositionEdgeVert(bottomVertices, midVertices, parameters, w,
                              dampingFactor, bndEnt, i/2, i == column.size()-2,
                              direction);
      computePositionEdgeVert(bottomVertices, topVertices, parameters, w,
                              dampingFactor, bndEnt, i/2, i == column.size()-2);
      repositionInteriorNodes(tri0);
      repositionInteriorNodes(tri1);
      bottom = MEdge(topVertices[0], topVertices[1]);

      // Check validity of first and last layer:
      if ((i == 0 || i == column.size()-2) &&
          (tri0->getValidity() != 1 || tri1->getValidity() != 1)) return false;
    }
    return true;
  }

  bool curve2DQuadColumn(MElement *bottomEdge, std::vector<MElement *> &column,
                         SVector3 w, double dampingFactor, GEntity *bndEnt)
  {
//    if (bottomEdge->getNum() != 1156) return true; // Strange
//    if (bottomEdge->getNum() != 1079) return true; // Good
//    if (bottomEdge->getNum() != 1136) return true; // Bad
//    if (bottomEdge->getNum() != 1102) return true; // HO

    MEdge bottom(bottomEdge->getVertex(0), bottomEdge->getVertex(1));
    std::vector<MVertex *> bottomVertices, topVertices;
    for (int i = 0; i < column.size(); ++i) {
      MQuadrangle *quad = dynamic_cast<MQuadrangle *>(column[i]);
      int iBottom, sign;
      quad->getEdgeInfo(bottom, iBottom, sign);
      if (iBottom != 0) {
        quad->reorient(4 - iBottom, false);
      }
      quad->getEdgeVertices(0, bottomVertices);
      quad->getEdgeVertices(2, topVertices);
      std::reverse(topVertices.begin(), topVertices.begin() + 2);
      std::reverse(topVertices.begin() + 2, topVertices.end());

      Parameters2DCurve parameters;
      computeExtremityCoefficients(bottomVertices, topVertices, parameters, w);
      computePositionEdgeVert(bottomVertices, topVertices, parameters, w,
                              dampingFactor, bndEnt, i, i == column.size()-1);
      repositionInteriorNodes(quad);
      bottom = MEdge(topVertices[0], topVertices[1]);

      // Check validity of first and last layer:
      if ((i == 0 || i == column.size()-1) && quad->getValidity() != 1) return false;

//      if (false && i < 16) {
//        double fact = 10;
//        for (int j = 1; j < 7; ++j) {
//          drawBezierDerivative(bottomVertices, bndEnt, SPoint3(fact*j, -10, 0), &j, .01);
//          drawBezierControlPolygon(bottomVertices, bndEnt);
//          if (i == column.size()-1) {
//            drawBezierDerivative(topVertices, bndEnt, SPoint3(fact*(j), -10, 0), &j, .01);
//            drawBezierControlPolygon(topVertices, bndEnt);
//          }
//        }
//      }
    }
    return true;
  }

}


void curve2DBoundaryLayer(VecPairMElemVecMElem &bndEl2column,
                          std::vector<MElement*> aboveElements,
                          SVector3 n, GEntity *bndEnt)
{
#ifdef _OPENMP
#pragma omp for
#endif
  for (int i = 0; i < bndEl2column.size(); ++i) {
    MElement *bottomEdge = bndEl2column[i].first;
    std::vector<MElement*> &column = bndEl2column[i].second;
//    std::cout << bottomEdge->getNum();
    double dampingFactor = 0;
    bool success = false;
    while (!success && dampingFactor < 1000) {
      if (column[0]->getType() == TYPE_TRI)
        success = BoundaryLayerCurver::curve2DTriColumn(bottomEdge,
                                                        aboveElements[i],
                                                        column, n,
                                                        dampingFactor, bndEnt);
      else
        success = BoundaryLayerCurver::curve2DQuadColumn(bottomEdge, column,
                                                         n, dampingFactor,
                                                         bndEnt);
      if (dampingFactor == 0) dampingFactor = .01;
      else dampingFactor *= 2;
    }
//    if (success)
//      std::cout << " ok for " << dampingFactor << std::endl;
//    else
//      std::cout << " NOT OK :'( " << dampingFactor << std::endl;
  }
}

void curve3DBoundaryLayer(VecPairMElemVecMElem &bndEl2column)
{

}
