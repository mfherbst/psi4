/*
 * @BEGIN LICENSE
 *
 * Psi4: an open-source quantum chemistry software package
 *
 * Copyright (c) 2007-2021 The Psi4 Developers.
 *
 * The copyrights for code used from other parties are included in
 * the corresponding files.
 *
 * This file is part of Psi4.
 *
 * Psi4 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Psi4 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with Psi4; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @END LICENSE
 */

#include "dct.h"
#include "psi4/psifiles.h"

#include "psi4/libpsi4util/PsiOutStream.h"
#include "psi4/liboptions/liboptions.h"
#include "psi4/libpsi4util/process.h"
#include "psi4/libtrans/integraltransform.h"
#include "psi4/libpsio/psio.hpp"
#include "psi4/libqt/qt.h"
#include "psi4/libiwl/iwl.h"
#include "psi4/libdiis/diismanager.h"
#include "psi4/libmints/molecule.h"
#include "psi4/libmints/oeprop.h"
#include "psi4/libmints/writer.h"
#include "psi4/libmints/writer_file_prefix.h"

namespace psi {
namespace dct {

/* Functions in this file exist to assemble the "relaxed" 2RDMs
 * needed in DC-06 analytic gradient theory. (DOI: 10.1063/1.4739423)
 * Both orbital response and amplitude response terms must be added to
 * the 2RDMs automatically delivered by the theory. Those are computed
 * in dct_density_[U/R]HF.cc. Gradients for ODC-06 and ODC-12 do not
 * need to distinguish between the two densities.
 */

void DCTSolver::compute_relaxed_density_OOOO() {
    psio_->open(PSIF_DCT_DENSITY, PSIO_OPEN_OLD);

    dpdbuf4 Zaa, Zab, Zbb, Laa, Lab, Lbb, Gaa, Gab, Gbb;

    // Compute the N^6 terms for Gamma OOOO

    // Gamma_ijkl = 1/16 (Lambda_ijab * Z_klab + Z_ijab * Lambda_klab)
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O>O]-"), ID("[O>O]-"), ID("[O>O]-"), ID("[O>O]-"), 0,
                           "Gamma <OO|OO>");
    global_dpd_->buf4_init(&Laa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Lambda <OO|VV>");
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Z <OO|VV>");
    global_dpd_->contract444(&Laa, &Zaa, &Gaa, 0, 0, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gaa);
    global_dpd_->buf4_close(&Zaa);
    global_dpd_->buf4_close(&Gaa);
    global_dpd_->buf4_close(&Laa);

    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0,
                           "Lambda <Oo|Vv>");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0, "Z <Oo|Vv>");
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), 0,
                           "Gamma <Oo|Oo>");
    global_dpd_->contract444(&Lab, &Zab, &Gab, 0, 0, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gab);
    global_dpd_->buf4_close(&Gab);
    global_dpd_->buf4_close(&Zab);
    global_dpd_->buf4_close(&Lab);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o>o]-"), ID("[o>o]-"), ID("[o>o]-"), ID("[o>o]-"), 0,
                           "Gamma <oo|oo>");
    global_dpd_->buf4_init(&Lbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Lambda <oo|vv>");
    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Z <oo|vv>");
    global_dpd_->contract444(&Lbb, &Zbb, &Gbb, 0, 0, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gbb);
    global_dpd_->buf4_close(&Zbb);
    global_dpd_->buf4_close(&Gbb);
    global_dpd_->buf4_close(&Lbb);

    /*
     * The OOOO  block
     */
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O,O]"), ID("[O,O]"), ID("[O>O]-"), ID("[O>O]-"), 0,
                           "Gamma <OO|OO>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gaa, h);
        global_dpd_->buf4_mat_irrep_rd(&Gaa, h);

#pragma omp parallel for
        for (size_t ij = 0; ij < Gaa.params->rowtot[h]; ++ij) {
            size_t i = Gaa.params->roworb[h][ij][0];
            int Gi = Gaa.params->psym[i];
            i -= Gaa.params->poff[Gi];
            size_t j = Gaa.params->roworb[h][ij][1];
            int Gj = Gaa.params->qsym[j];
            j -= Gaa.params->qoff[Gj];
            for (size_t kl = 0; kl < Gaa.params->coltot[h]; ++kl) {
                double tpdm = 0.0;
                size_t k = Gaa.params->colorb[h][kl][0];
                int Gk = Gaa.params->rsym[k];
                k -= Gaa.params->roff[Gk];
                size_t l = Gaa.params->colorb[h][kl][1];
                int Gl = Gaa.params->ssym[l];
                l -= Gaa.params->soff[Gl];

                if (Gi == Gk && Gj == Gl) tpdm += 0.25 * kappa_mo_a_->get(Gi, i, k) * kappa_mo_a_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk) tpdm -= 0.25 * kappa_mo_a_->get(Gi, i, l) * kappa_mo_a_->get(Gj, j, k);

                if (Gi == Gk && Gj == Gl)
                    tpdm += 0.25 * (kappa_mo_a_->get(Gi, i, k) + aocc_tau_->get(Gi, i, k)) * aocc_ptau_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk)
                    tpdm -= 0.25 * (kappa_mo_a_->get(Gi, i, l) + aocc_tau_->get(Gi, i, l)) * aocc_ptau_->get(Gj, j, k);
                if (Gj == Gk && Gi == Gl)
                    tpdm -= 0.25 * (kappa_mo_a_->get(Gj, j, k) + aocc_tau_->get(Gj, j, k)) * aocc_ptau_->get(Gi, i, l);
                if (Gj == Gl && Gi == Gk)
                    tpdm += 0.25 * (kappa_mo_a_->get(Gj, j, l) + aocc_tau_->get(Gj, j, l)) * aocc_ptau_->get(Gi, i, k);

                if (Gi == Gk && Gj == Gl) tpdm -= 0.25 * aocc_tau_->get(Gi, i, k) * aocc_tau_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk) tpdm += 0.25 * aocc_tau_->get(Gi, i, l) * aocc_tau_->get(Gj, j, k);

                Gaa.matrix[h][ij][kl] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gaa, h);
        global_dpd_->buf4_mat_irrep_close(&Gaa, h);
    }

    global_dpd_->buf4_close(&Gaa);

    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), 0,
                           "Gamma <Oo|Oo>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gab, h);
        global_dpd_->buf4_mat_irrep_rd(&Gab, h);

#pragma omp parallel for
        for (size_t ij = 0; ij < Gab.params->rowtot[h]; ++ij) {
            size_t i = Gab.params->roworb[h][ij][0];
            int Gi = Gab.params->psym[i];
            i -= Gab.params->poff[Gi];
            size_t j = Gab.params->roworb[h][ij][1];
            int Gj = Gab.params->qsym[j];
            j -= Gab.params->qoff[Gj];
            for (size_t kl = 0; kl < Gab.params->coltot[h]; ++kl) {
                double tpdm = 0.0;
                size_t k = Gab.params->colorb[h][kl][0];
                int Gk = Gab.params->rsym[k];
                k -= Gab.params->roff[Gk];
                size_t l = Gab.params->colorb[h][kl][1];
                int Gl = Gab.params->ssym[l];
                l -= Gab.params->soff[Gl];
                if (Gi == Gk && Gj == Gl) tpdm += 0.25 * kappa_mo_a_->get(Gi, i, k) * kappa_mo_b_->get(Gj, j, l);

                if (Gi == Gk && Gj == Gl)
                    tpdm += 0.25 * (kappa_mo_a_->get(Gi, i, k) + aocc_tau_->get(Gi, i, k)) * bocc_ptau_->get(Gj, j, l);
                if (Gj == Gl && Gi == Gk)
                    tpdm += 0.25 * (kappa_mo_b_->get(Gj, j, l) + bocc_tau_->get(Gj, j, l)) * aocc_ptau_->get(Gi, i, k);

                if (Gi == Gk && Gj == Gl) tpdm -= 0.25 * aocc_tau_->get(Gi, i, k) * bocc_tau_->get(Gj, j, l);

                Gab.matrix[h][ij][kl] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gab, h);
        global_dpd_->buf4_mat_irrep_close(&Gab, h);
    }

    global_dpd_->buf4_close(&Gab);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o,o]"), ID("[o,o]"), ID("[o>o]-"), ID("[o>o]-"), 0,
                           "Gamma <oo|oo>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gbb, h);
        global_dpd_->buf4_mat_irrep_rd(&Gbb, h);

#pragma omp parallel for
        for (size_t ij = 0; ij < Gbb.params->rowtot[h]; ++ij) {
            size_t i = Gbb.params->roworb[h][ij][0];
            int Gi = Gbb.params->psym[i];
            i -= Gbb.params->poff[Gi];
            size_t j = Gbb.params->roworb[h][ij][1];
            int Gj = Gbb.params->qsym[j];
            j -= Gbb.params->qoff[Gj];
            for (size_t kl = 0; kl < Gbb.params->coltot[h]; ++kl) {
                double tpdm = 0.0;
                size_t k = Gbb.params->colorb[h][kl][0];
                int Gk = Gbb.params->rsym[k];
                k -= Gbb.params->roff[Gk];
                size_t l = Gbb.params->colorb[h][kl][1];
                int Gl = Gbb.params->ssym[l];
                l -= Gbb.params->soff[Gl];
                if (Gi == Gk && Gj == Gl) tpdm += 0.25 * kappa_mo_b_->get(Gi, i, k) * kappa_mo_b_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk) tpdm -= 0.25 * kappa_mo_b_->get(Gi, i, l) * kappa_mo_b_->get(Gj, j, k);

                if (Gi == Gk && Gj == Gl)
                    tpdm += 0.25 * (kappa_mo_b_->get(Gi, i, k) + bocc_tau_->get(Gi, i, k)) * bocc_ptau_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk)
                    tpdm -= 0.25 * (kappa_mo_b_->get(Gi, i, l) + bocc_tau_->get(Gi, i, l)) * bocc_ptau_->get(Gj, j, k);
                if (Gj == Gk && Gi == Gl)
                    tpdm -= 0.25 * (kappa_mo_b_->get(Gj, j, k) + bocc_tau_->get(Gj, j, k)) * bocc_ptau_->get(Gi, i, l);
                if (Gj == Gl && Gi == Gk)
                    tpdm += 0.25 * (kappa_mo_b_->get(Gj, j, l) + bocc_tau_->get(Gj, j, l)) * bocc_ptau_->get(Gi, i, k);

                if (Gi == Gk && Gj == Gl) tpdm -= 0.25 * bocc_tau_->get(Gi, i, k) * bocc_tau_->get(Gj, j, l);
                if (Gi == Gl && Gj == Gk) tpdm += 0.25 * bocc_tau_->get(Gi, i, l) * bocc_tau_->get(Gj, j, k);

                Gbb.matrix[h][ij][kl] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gbb, h);
        global_dpd_->buf4_mat_irrep_close(&Gbb, h);
    }

    global_dpd_->buf4_close(&Gbb);

    psio_->close(PSIF_DCT_DENSITY, 1);
}

void DCTSolver::compute_relaxed_density_OOVV() {
    psio_->open(PSIF_DCT_DENSITY, PSIO_OPEN_OLD);

    dpdbuf4 Zaa, Zab, Zbb, Laa, Lab, Lbb, Gaa, Gab, Gbb;

    /*
     * The OOVV and VVOO blocks
     */

    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Gamma <OO|VV>");
    global_dpd_->buf4_init(&Laa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Lambda <OO|VV>");
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Z <OO|VV>");
    global_dpd_->buf4_axpbycz(&Laa, &Zaa, &Gaa, 0.25, 0.25, 0.0);
    global_dpd_->buf4_close(&Zaa);
    global_dpd_->buf4_close(&Laa);
    global_dpd_->buf4_close(&Gaa);

    // Resort the Г_OOVV to Г_VVOO. Used for the MO Lagrangian
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Gamma <OO|VV>");
    global_dpd_->buf4_sort(&Gaa, PSIF_DCT_DENSITY, rspq, ID("[V>V]-"), ID("[O>O]-"), "Gamma <VV|OO>");
    global_dpd_->buf4_close(&Gaa);

    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0,
                           "Lambda <Oo|Vv>");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0, "Z <Oo|Vv>");
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0,
                           "Gamma <Oo|Vv>");
    global_dpd_->buf4_axpbycz(&Lab, &Zab, &Gab, 0.25, 0.25, 0.0);
    global_dpd_->buf4_close(&Zab);
    global_dpd_->buf4_close(&Lab);
    global_dpd_->buf4_close(&Gab);

    // Resort the Г_OoVv to Г_VvOo. Used for the MO Lagrangian
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0,
                           "Gamma <Oo|Vv>");
    global_dpd_->buf4_sort(&Gab, PSIF_DCT_DENSITY, rspq, ID("[V,v]"), ID("[O,o]"), "Gamma <Vv|Oo>");
    global_dpd_->buf4_close(&Gab);

    global_dpd_->buf4_init(&Lbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Lambda <oo|vv>");
    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Z <oo|vv>");
    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Gamma <oo|vv>");
    global_dpd_->buf4_axpbycz(&Lbb, &Zbb, &Gbb, 0.25, 0.25, 0.0);
    global_dpd_->buf4_close(&Gbb);
    global_dpd_->buf4_close(&Zbb);
    global_dpd_->buf4_close(&Lbb);

    // Resort the Г_oovv to Г_vvoo. Used for the MO Lagrangian
    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Gamma <oo|vv>");
    global_dpd_->buf4_sort(&Gbb, PSIF_DCT_DENSITY, rspq, ID("[v>v]-"), ID("[o>o]-"), "Gamma <vv|oo>");
    global_dpd_->buf4_close(&Gbb);

    psio_->close(PSIF_DCT_DENSITY, 1);
}

void DCTSolver::compute_relaxed_density_OVOV() {
    psio_->open(PSIF_DCT_DENSITY, PSIO_OPEN_OLD);

    dpdbuf4 Zaa, Zab, Zbb, Laa, Lab, Lbb, Gaa, Gab, Gba, Gbb, Tab;

    /*
     * The OVOV block
     */

    // There are five unique spin cases: Г<IAJB>, Г<iajb>, Г<IaJb>, Г<iAjB>, Г<IajB>

    // TEMPORARY: Sort the cumulant Z-vector elements to chemist's notation.
    // MOVE THIS TO THE Z-VECTOR UPDATES WHEN NEEDED!!!
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O,O]"), ID("[V,V]"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Z <OO|VV>");
    global_dpd_->buf4_sort(&Zaa, PSIF_DCT_DPD, prqs, ID("[O,V]"), ID("[O,V]"), "Z (OV|OV)");
    global_dpd_->buf4_close(&Zaa);

    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0, "Z <Oo|Vv>");
    global_dpd_->buf4_sort(&Zab, PSIF_DCT_DPD, psqr, ID("[O,v]"), ID("[o,V]"), "Z (Ov|oV)");
    global_dpd_->buf4_close(&Zab);

    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o,o]"), ID("[v,v]"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Z <oo|vv>");
    global_dpd_->buf4_sort(&Zbb, PSIF_DCT_DPD, prqs, ID("[o,v]"), ID("[o,v]"), "Z (ov|ov)");
    global_dpd_->buf4_close(&Zbb);

    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,v]"), ID("[o,V]"), ID("[O,v]"), ID("[o,V]"), 0, "Z (Ov|oV)");

    global_dpd_->buf4_sort(&Zab, PSIF_DCT_DPD, psrq, ID("[O,V]"), ID("[o,v]"), "Z (OV|ov)");
    global_dpd_->buf4_close(&Zab);

    // Г<IAJB> spin case

    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Gamma (OV|OV)");
    global_dpd_->buf4_init(&Laa, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Lambda (OV|OV)");
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0, "Z (OV|OV)");
    global_dpd_->contract444(&Laa, &Zaa, &Gaa, 0, 0, -1.0, 0.0);
    global_dpd_->buf4_close(&Laa);
    global_dpd_->buf4_close(&Zaa);
    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0,
                           "Lambda (OV|ov)");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0, "Z (OV|ov)");
    global_dpd_->contract444(&Lab, &Zab, &Gaa, 0, 0, -1.0, 1.0);
    global_dpd_->buf4_close(&Lab);
    global_dpd_->buf4_close(&Zab);
    global_dpd_->buf4_close(&Gaa);
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Gamma (OV|OV)");
    global_dpd_->buf4_symm(&Gaa);
    global_dpd_->buf4_close(&Gaa);

    // Resort Г(OV|OV) to the Г<OV|OV>
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Gamma (OV|OV)");
    global_dpd_->buf4_sort(&Gaa, PSIF_DCT_DENSITY, psrq, ID("[O,V]"), ID("[O,V]"), "Gamma <OV|OV>");
    global_dpd_->buf4_close(&Gaa);

    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Gamma <OV|OV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gaa, h);
        global_dpd_->buf4_mat_irrep_rd(&Gaa, h);

#pragma omp parallel for
        for (size_t ia = 0; ia < Gaa.params->rowtot[h]; ++ia) {
            size_t i = Gaa.params->roworb[h][ia][0];
            int Gi = Gaa.params->psym[i];
            i -= Gaa.params->poff[Gi];
            size_t a = Gaa.params->roworb[h][ia][1];
            int Ga = Gaa.params->qsym[a];
            a -= Gaa.params->qoff[Ga];
            for (size_t jb = 0; jb < Gaa.params->coltot[h]; ++jb) {
                size_t j = Gaa.params->colorb[h][jb][0];
                int Gj = Gaa.params->rsym[j];
                j -= Gaa.params->roff[Gj];
                size_t b = Gaa.params->colorb[h][jb][1];
                int Gb = Gaa.params->ssym[b];
                b -= Gaa.params->soff[Gb];
                if (Gi == Gj && Ga == Gb) {
                    Gaa.matrix[h][ia][jb] +=
                        (kappa_mo_a_->get(Gi, i, j) + aocc_tau_->get(Gi, i, j)) * avir_ptau_->get(Ga, a, b);
                    Gaa.matrix[h][ia][jb] +=
                        avir_tau_->get(Ga, a, b) * (aocc_ptau_->get(Gi, i, j) - aocc_tau_->get(Gi, i, j));
                }
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gaa, h);
        global_dpd_->buf4_mat_irrep_close(&Gaa, h);
    }

    global_dpd_->buf4_close(&Gaa);

    // Г<IaJb> and Г<iAjB> spin cases:

    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,v]"), ID("[o,V]"), ID("[O,v]"), ID("[o,V]"), 0,
                           "Lambda (Ov|oV)");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,v]"), ID("[o,V]"), ID("[O,v]"), ID("[o,V]"), 0, "Z (Ov|oV)");
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), 0,
                           "Gamma <Ov|Ov>");
    global_dpd_->contract444(&Lab, &Zab, &Gab, 0, 0, -1.0, 0.0);
    global_dpd_->buf4_close(&Gab);
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), 0,
                           "Gamma <Ov|Ov>");
    global_dpd_->buf4_symm(&Gab);
    global_dpd_->buf4_close(&Gab);
    global_dpd_->buf4_init(&Gba, PSIF_DCT_DENSITY, 0, ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), 0,
                           "Gamma <oV|oV>");
    global_dpd_->contract444(&Lab, &Zab, &Gba, 1, 1, -1.0, 0.0);
    global_dpd_->buf4_close(&Gba);
    global_dpd_->buf4_init(&Gba, PSIF_DCT_DENSITY, 0, ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), 0,
                           "Gamma <oV|oV>");
    global_dpd_->buf4_symm(&Gba);
    global_dpd_->buf4_close(&Gba);
    global_dpd_->buf4_close(&Lab);
    global_dpd_->buf4_close(&Zab);

    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), 0,
                           "Gamma <Ov|Ov>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gab, h);
        global_dpd_->buf4_mat_irrep_rd(&Gab, h);

#pragma omp parallel for
        for (size_t ia = 0; ia < Gab.params->rowtot[h]; ++ia) {
            size_t i = Gab.params->roworb[h][ia][0];
            int Gi = Gab.params->psym[i];
            i -= Gab.params->poff[Gi];
            size_t a = Gab.params->roworb[h][ia][1];
            int Ga = Gab.params->qsym[a];
            a -= Gab.params->qoff[Ga];
            for (size_t jb = 0; jb < Gab.params->coltot[h]; ++jb) {
                size_t j = Gab.params->colorb[h][jb][0];
                int Gj = Gab.params->rsym[j];
                j -= Gab.params->roff[Gj];
                size_t b = Gab.params->colorb[h][jb][1];
                int Gb = Gab.params->ssym[b];
                b -= Gab.params->soff[Gb];
                if (Gi == Gj && Ga == Gb) {
                    Gab.matrix[h][ia][jb] +=
                        (kappa_mo_a_->get(Gi, i, j) + aocc_tau_->get(Gi, i, j)) * bvir_ptau_->get(Ga, a, b);
                    Gab.matrix[h][ia][jb] +=
                        bvir_tau_->get(Ga, a, b) * (aocc_ptau_->get(Gi, i, j) - aocc_tau_->get(Gi, i, j));
                }
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gab, h);
        global_dpd_->buf4_mat_irrep_close(&Gab, h);
    }

    global_dpd_->buf4_close(&Gab);

    global_dpd_->buf4_init(&Gba, PSIF_DCT_DENSITY, 0, ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), 0,
                           "Gamma <oV|oV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gba, h);
        global_dpd_->buf4_mat_irrep_rd(&Gba, h);

#pragma omp parallel for
        for (size_t ia = 0; ia < Gba.params->rowtot[h]; ++ia) {
            size_t i = Gba.params->roworb[h][ia][0];
            int Gi = Gba.params->psym[i];
            i -= Gba.params->poff[Gi];
            size_t a = Gba.params->roworb[h][ia][1];
            int Ga = Gba.params->qsym[a];
            a -= Gba.params->qoff[Ga];
            for (size_t jb = 0; jb < Gba.params->coltot[h]; ++jb) {
                size_t j = Gba.params->colorb[h][jb][0];
                int Gj = Gba.params->rsym[j];
                j -= Gba.params->roff[Gj];
                size_t b = Gba.params->colorb[h][jb][1];
                int Gb = Gba.params->ssym[b];
                b -= Gba.params->soff[Gb];
                if (Gi == Gj && Ga == Gb) {
                    Gba.matrix[h][ia][jb] +=
                        (kappa_mo_b_->get(Gi, i, j) + bocc_tau_->get(Gi, i, j)) * avir_ptau_->get(Ga, a, b);
                    Gba.matrix[h][ia][jb] +=
                        avir_tau_->get(Ga, a, b) * (bocc_ptau_->get(Gi, i, j) - bocc_tau_->get(Gi, i, j));
                }
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gba, h);
        global_dpd_->buf4_mat_irrep_close(&Gba, h);
    }

    global_dpd_->buf4_close(&Gba);

    // Г<IajB> spin case:

    global_dpd_->buf4_init(&Tab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0,
                           "Temp (OV|ov)");
    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0,
                           "Lambda (OV|ov)");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0, "Z (OV|ov)");
    global_dpd_->buf4_init(&Laa, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Lambda (OV|OV)");
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0, "Z (OV|OV)");
    global_dpd_->contract444(&Laa, &Zab, &Tab, 0, 1, -0.5, 0.0);
    global_dpd_->contract444(&Zaa, &Lab, &Tab, 0, 1, -0.5, 1.0);
    global_dpd_->buf4_close(&Laa);
    global_dpd_->buf4_close(&Zaa);
    global_dpd_->buf4_init(&Lbb, PSIF_DCT_DPD, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Lambda (ov|ov)");
    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0, "Z (ov|ov)");
    global_dpd_->contract444(&Lab, &Zbb, &Tab, 0, 1, -0.5, 1.0);
    global_dpd_->contract444(&Zab, &Lbb, &Tab, 0, 1, -0.5, 1.0);
    global_dpd_->buf4_close(&Lbb);
    global_dpd_->buf4_close(&Zbb);
    global_dpd_->buf4_close(&Tab);
    global_dpd_->buf4_init(&Tab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0,
                           "Temp (OV|ov)");
    global_dpd_->buf4_sort(&Tab, PSIF_DCT_DENSITY, psrq, ID("[O,v]"), ID("[o,V]"), "Gamma <Ov|oV>");
    // Resort to get the Г_oVOv. Used for the MO Lagrangian
    global_dpd_->buf4_sort(&Tab, PSIF_DCT_DENSITY, rqps, ID("[o,V]"), ID("[O,v]"), "Gamma <oV|Ov>");

    global_dpd_->buf4_close(&Tab);
    global_dpd_->buf4_close(&Lab);
    global_dpd_->buf4_close(&Zab);

    // Г<iajb> spin case:

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Gamma (ov|ov)");
    global_dpd_->buf4_init(&Lbb, PSIF_DCT_DPD, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Lambda (ov|ov)");
    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0, "Z (ov|ov)");
    global_dpd_->contract444(&Lbb, &Zbb, &Gbb, 0, 0, -1.0, 0.0);
    global_dpd_->buf4_close(&Lbb);
    global_dpd_->buf4_close(&Zbb);
    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0,
                           "Lambda (OV|ov)");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,V]"), ID("[o,v]"), ID("[O,V]"), ID("[o,v]"), 0, "Z (OV|ov)");
    global_dpd_->contract444(&Lab, &Zab, &Gbb, 1, 1, -1.0, 1.0);
    global_dpd_->buf4_close(&Lab);
    global_dpd_->buf4_close(&Zab);
    global_dpd_->buf4_close(&Gbb);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Gamma (ov|ov)");
    global_dpd_->buf4_symm(&Gbb);
    global_dpd_->buf4_close(&Gbb);

    // Resort Г(ov|ov) to the Г<ov|ov>
    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Gamma (ov|ov)");
    global_dpd_->buf4_sort(&Gbb, PSIF_DCT_DENSITY, psrq, ID("[o,v]"), ID("[o,v]"), "Gamma <ov|ov>");
    global_dpd_->buf4_close(&Gbb);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Gamma <ov|ov>");

    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gbb, h);
        global_dpd_->buf4_mat_irrep_rd(&Gbb, h);

#pragma omp parallel for
        for (size_t ia = 0; ia < Gbb.params->rowtot[h]; ++ia) {
            size_t i = Gbb.params->roworb[h][ia][0];
            int Gi = Gbb.params->psym[i];
            i -= Gbb.params->poff[Gi];
            size_t a = Gbb.params->roworb[h][ia][1];
            int Ga = Gbb.params->qsym[a];
            a -= Gbb.params->qoff[Ga];
            for (size_t jb = 0; jb < Gbb.params->coltot[h]; ++jb) {
                size_t j = Gbb.params->colorb[h][jb][0];
                int Gj = Gbb.params->rsym[j];
                j -= Gbb.params->roff[Gj];
                size_t b = Gbb.params->colorb[h][jb][1];
                int Gb = Gbb.params->ssym[b];
                b -= Gbb.params->soff[Gb];
                if (Gi == Gj && Ga == Gb) {
                    Gbb.matrix[h][ia][jb] +=
                        (kappa_mo_b_->get(Gi, i, j) + bocc_tau_->get(Gi, i, j)) * bvir_ptau_->get(Ga, a, b);
                    Gbb.matrix[h][ia][jb] +=
                        bvir_tau_->get(Ga, a, b) * (bocc_ptau_->get(Gi, i, j) - bocc_tau_->get(Gi, i, j));
                }
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gbb, h);
        global_dpd_->buf4_mat_irrep_close(&Gbb, h);
    }

    global_dpd_->buf4_close(&Gbb);

    psio_->close(PSIF_DCT_DENSITY, 1);
}

void DCTSolver::compute_relaxed_density_VVVV() {
    psio_->open(PSIF_DCT_DENSITY, PSIO_OPEN_OLD);

    dpdbuf4 Zaa, Zab, Zbb, Laa, Lab, Lbb, Gaa, Gab, Gbb;

    /*
     * The VVVV block
     */

    // Gamma_abcd = 1/16 (Lambda_ijab * Z_ijcd + Z_ijab * Lambda_ijcd)
    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[V>V]-"), ID("[V>V]-"), ID("[V>V]-"), ID("[V>V]-"), 0,
                           "Gamma <VV|VV>");
    global_dpd_->buf4_init(&Laa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Lambda <OO|VV>");
    global_dpd_->buf4_init(&Zaa, PSIF_DCT_DPD, 0, ID("[O>O]-"), ID("[V>V]-"), ID("[O>O]-"), ID("[V>V]-"), 0,
                           "Z <OO|VV>");
    global_dpd_->contract444(&Laa, &Zaa, &Gaa, 1, 1, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gaa);
    global_dpd_->buf4_close(&Zaa);
    global_dpd_->buf4_close(&Gaa);
    global_dpd_->buf4_close(&Laa);

    global_dpd_->buf4_init(&Lab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0,
                           "Lambda <Oo|Vv>");
    global_dpd_->buf4_init(&Zab, PSIF_DCT_DPD, 0, ID("[O,o]"), ID("[V,v]"), ID("[O,o]"), ID("[V,v]"), 0, "Z <Oo|Vv>");
    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), 0,
                           "Gamma <Vv|Vv>");
    global_dpd_->contract444(&Lab, &Zab, &Gab, 1, 1, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gab);
    global_dpd_->buf4_close(&Gab);
    global_dpd_->buf4_close(&Zab);
    global_dpd_->buf4_close(&Lab);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[v>v]-"), ID("[v>v]-"), ID("[v>v]-"), ID("[v>v]-"), 0,
                           "Gamma <vv|vv>");
    global_dpd_->buf4_init(&Lbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Lambda <oo|vv>");
    global_dpd_->buf4_init(&Zbb, PSIF_DCT_DPD, 0, ID("[o>o]-"), ID("[v>v]-"), ID("[o>o]-"), ID("[v>v]-"), 0,
                           "Z <oo|vv>");
    global_dpd_->contract444(&Lbb, &Zbb, &Gbb, 1, 1, 0.25, 0.0);
    global_dpd_->buf4_symm(&Gbb);
    global_dpd_->buf4_close(&Zbb);
    global_dpd_->buf4_close(&Gbb);
    global_dpd_->buf4_close(&Lbb);

    global_dpd_->buf4_init(&Gaa, PSIF_DCT_DENSITY, 0, ID("[V,V]"), ID("[V,V]"), ID("[V>V]-"), ID("[V>V]-"), 0,
                           "Gamma <VV|VV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gaa, h);
        global_dpd_->buf4_mat_irrep_rd(&Gaa, h);

#pragma omp parallel for
        for (size_t ab = 0; ab < Gaa.params->rowtot[h]; ++ab) {
            size_t a = Gaa.params->roworb[h][ab][0];
            int Ga = Gaa.params->psym[a];
            a -= Gaa.params->poff[Ga];
            size_t b = Gaa.params->roworb[h][ab][1];
            int Gb = Gaa.params->qsym[b];
            b -= Gaa.params->qoff[Gb];
            for (size_t cd = 0; cd < Gaa.params->coltot[h]; ++cd) {
                double tpdm = 0.0;
                size_t c = Gaa.params->colorb[h][cd][0];
                int Gc = Gaa.params->rsym[c];
                c -= Gaa.params->roff[Gc];
                size_t d = Gaa.params->colorb[h][cd][1];
                int Gd = Gaa.params->ssym[d];
                d -= Gaa.params->soff[Gd];
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * avir_tau_->get(Ga, a, c) * avir_ptau_->get(Gb, b, d);
                if (Ga == Gd && Gb == Gc) tpdm -= 0.25 * avir_tau_->get(Ga, a, d) * avir_ptau_->get(Gb, b, c);
                if (Gb == Gc && Ga == Gd) tpdm -= 0.25 * avir_tau_->get(Gb, b, c) * avir_ptau_->get(Ga, a, d);
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * avir_tau_->get(Gb, b, d) * avir_ptau_->get(Ga, a, c);

                if (Ga == Gc && Gb == Gd) tpdm -= 0.25 * avir_tau_->get(Ga, a, c) * avir_tau_->get(Gb, b, d);
                if (Ga == Gd && Gb == Gc) tpdm += 0.25 * avir_tau_->get(Ga, a, d) * avir_tau_->get(Gb, b, c);

                Gaa.matrix[h][ab][cd] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gaa, h);
        global_dpd_->buf4_mat_irrep_close(&Gaa, h);
    }

    global_dpd_->buf4_close(&Gaa);

    global_dpd_->buf4_init(&Gab, PSIF_DCT_DENSITY, 0, ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), 0,
                           "Gamma <Vv|Vv>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gab, h);
        global_dpd_->buf4_mat_irrep_rd(&Gab, h);

#pragma omp parallel for
        for (size_t ab = 0; ab < Gab.params->rowtot[h]; ++ab) {
            size_t a = Gab.params->roworb[h][ab][0];
            int Ga = Gab.params->psym[a];
            a -= Gab.params->poff[Ga];
            size_t b = Gab.params->roworb[h][ab][1];
            int Gb = Gab.params->qsym[b];
            b -= Gab.params->qoff[Gb];
            for (size_t cd = 0; cd < Gab.params->coltot[h]; ++cd) {
                double tpdm = 0.0;
                size_t c = Gab.params->colorb[h][cd][0];
                int Gc = Gab.params->rsym[c];
                c -= Gab.params->roff[Gc];
                size_t d = Gab.params->colorb[h][cd][1];
                int Gd = Gab.params->ssym[d];
                d -= Gab.params->soff[Gd];
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * avir_tau_->get(Ga, a, c) * bvir_ptau_->get(Gb, b, d);
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * bvir_tau_->get(Gb, b, d) * avir_ptau_->get(Ga, a, c);

                if (Ga == Gc && Gb == Gd) tpdm -= 0.25 * avir_tau_->get(Ga, a, c) * bvir_tau_->get(Gb, b, d);
                Gab.matrix[h][ab][cd] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gab, h);
        global_dpd_->buf4_mat_irrep_close(&Gab, h);
    }

    global_dpd_->buf4_close(&Gab);

    global_dpd_->buf4_init(&Gbb, PSIF_DCT_DENSITY, 0, ID("[v,v]"), ID("[v,v]"), ID("[v>v]-"), ID("[v>v]-"), 0,
                           "Gamma <vv|vv>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&Gbb, h);
        global_dpd_->buf4_mat_irrep_rd(&Gbb, h);

#pragma omp parallel for
        for (size_t ab = 0; ab < Gbb.params->rowtot[h]; ++ab) {
            size_t a = Gbb.params->roworb[h][ab][0];
            int Ga = Gbb.params->psym[a];
            a -= Gbb.params->poff[Ga];
            size_t b = Gbb.params->roworb[h][ab][1];
            int Gb = Gbb.params->qsym[b];
            b -= Gbb.params->qoff[Gb];
            for (size_t cd = 0; cd < Gbb.params->coltot[h]; ++cd) {
                double tpdm = 0.0;
                size_t c = Gbb.params->colorb[h][cd][0];
                int Gc = Gbb.params->rsym[c];
                c -= Gbb.params->roff[Gc];
                size_t d = Gbb.params->colorb[h][cd][1];
                int Gd = Gbb.params->ssym[d];
                d -= Gbb.params->soff[Gd];
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * bvir_tau_->get(Ga, a, c) * bvir_ptau_->get(Gb, b, d);
                if (Ga == Gd && Gb == Gc) tpdm -= 0.25 * bvir_tau_->get(Ga, a, d) * bvir_ptau_->get(Gb, b, c);
                if (Gb == Gc && Ga == Gd) tpdm -= 0.25 * bvir_tau_->get(Gb, b, c) * bvir_ptau_->get(Ga, a, d);
                if (Ga == Gc && Gb == Gd) tpdm += 0.25 * bvir_tau_->get(Gb, b, d) * bvir_ptau_->get(Ga, a, c);

                if (Ga == Gc && Gb == Gd) tpdm -= 0.25 * bvir_tau_->get(Ga, a, c) * bvir_tau_->get(Gb, b, d);
                if (Ga == Gd && Gb == Gc) tpdm += 0.25 * bvir_tau_->get(Ga, a, d) * bvir_tau_->get(Gb, b, c);
                Gbb.matrix[h][ab][cd] += tpdm;
            }
        }
        global_dpd_->buf4_mat_irrep_wrt(&Gbb, h);
        global_dpd_->buf4_mat_irrep_close(&Gbb, h);
    }

    global_dpd_->buf4_close(&Gbb);

    psio_->close(PSIF_DCT_DENSITY, 1);
}

void DCTSolver::compute_TPDM_trace() {
    dpdbuf4 G;

    psio_->open(PSIF_DCT_DENSITY, PSIO_OPEN_OLD);

    double tpdm_trace = 0.0;

    // OOOO density
    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[O>O]-"), ID("[O>O]-"), ID("[O>O]-"), ID("[O>O]-"), 0,
                           "Gamma <OO|OO>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ij = 0; ij < G.params->rowtot[h]; ++ij) {
            tpdm_trace += 8.0 * G.matrix[h][ij][ij];
        }

        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }
    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), ID("[O,o]"), 0,
                           "Gamma <Oo|Oo>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ij = 0; ij < G.params->rowtot[h]; ++ij) {
            tpdm_trace += 8.0 * G.matrix[h][ij][ij];
        }

        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }
    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[o>o]-"), ID("[o>o]-"), ID("[o>o]-"), ID("[o>o]-"), 0,
                           "Gamma <oo|oo>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ij = 0; ij < G.params->rowtot[h]; ++ij) {
            tpdm_trace += 8.0 * G.matrix[h][ij][ij];
        }

        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }
    global_dpd_->buf4_close(&G);

    // VVVV density
    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[V>V]-"), ID("[V>V]-"), ID("[V>V]-"), ID("[V>V]-"), 0,
                           "Gamma <VV|VV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ab = 0; ab < G.params->rowtot[h]; ++ab) {
            tpdm_trace += 8.0 * G.matrix[h][ab][ab];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), ID("[V,v]"), 0,
                           "Gamma <Vv|Vv>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ab = 0; ab < G.params->rowtot[h]; ++ab) {
            tpdm_trace += 8.0 * G.matrix[h][ab][ab];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[v>v]-"), ID("[v>v]-"), ID("[v>v]-"), ID("[v>v]-"), 0,
                           "Gamma <vv|vv>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ab = 0; ab < G.params->rowtot[h]; ++ab) {
            tpdm_trace += 8.0 * G.matrix[h][ab][ab];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    // OVOV density
    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), ID("[O,V]"), 0,
                           "Gamma <OV|OV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ia = 0; ia < G.params->rowtot[h]; ++ia) {
            tpdm_trace += 2.0 * G.matrix[h][ia][ia];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), ID("[O,v]"), 0,
                           "Gamma <Ov|Ov>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ia = 0; ia < G.params->rowtot[h]; ++ia) {
            tpdm_trace += 2.0 * G.matrix[h][ia][ia];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), ID("[o,V]"), 0,
                           "Gamma <oV|oV>");
    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ia = 0; ia < G.params->rowtot[h]; ++ia) {
            tpdm_trace += 2.0 * G.matrix[h][ia][ia];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    global_dpd_->buf4_init(&G, PSIF_DCT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), ID("[o,v]"), 0,
                           "Gamma <ov|ov>");

    for (int h = 0; h < nirrep_; ++h) {
        global_dpd_->buf4_mat_irrep_init(&G, h);
        global_dpd_->buf4_mat_irrep_rd(&G, h);

        for (size_t ia = 0; ia < G.params->rowtot[h]; ++ia) {
            tpdm_trace += 2.0 * G.matrix[h][ia][ia];
        }
        global_dpd_->buf4_mat_irrep_wrt(&G, h);
        global_dpd_->buf4_mat_irrep_close(&G, h);
    }

    global_dpd_->buf4_close(&G);

    // Compute OPDM trace
    double opdm_trace = (kappa_mo_a_->trace() + kappa_mo_b_->trace());
    opdm_trace += (aocc_tau_->trace() + bocc_tau_->trace() + avir_tau_->trace() + bvir_tau_->trace());

    // Compute deviations from N-representability
    auto N = (double)(nalpha_ + nbeta_);
    double opdm_dev = N - opdm_trace;
    double tpdm_dev = N * (N - 1.0) - tpdm_trace;

    outfile->Printf("\t OPDM trace: \t%10.6f\t\tDeviation: \t%8.4e\n", opdm_trace, opdm_dev);
    outfile->Printf("\t TPDM trace: \t%10.6f\t\tDeviation: \t%8.4e\n", tpdm_trace, tpdm_dev);

    psio_->close(PSIF_DCT_DENSITY, 1);
}

} // namespace dct
} // namespace psi
