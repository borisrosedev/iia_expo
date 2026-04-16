from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QLineEdit, QTextEdit, QFrame
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QObject
from PyQt5.QtGui import QFont, QColor, QTextCursor, QTextCharFormat
import time

from cookie_tfo import (
    simuler_premiere_connexion,
    simuler_connexion_future,
    simuler_cookie_invalide,
)

# ─── Couleurs ────────────────────────────────────────────────────────────────
BG          = "#1e1e2e"
BG_PANEL    = "#2a2a3e"
BG_INPUT    = "#313145"
ACCENT      = "#7c3aed"
ACCENT_LITE = "#a78bfa"
CLIENT_CLR  = "#38bdf8"
SERVER_CLR  = "#34d399"
WARN_CLR    = "#f59e0b"
ERR_CLR     = "#f87171"
OK_CLR      = "#4ade80"
TEXT        = "#e2e8f0"
TEXT_DIM    = "#94a3b8"
BORDER      = "#3f3f5a"
SEP_CLR     = "#3f3f5a"

DELAY = 0.6  # secondes entre chaque événement animé

TYPE_COLOR = {
    "send":    None,       # résolu dynamiquement selon source
    "receive": None,
    "process": WARN_CLR,
    "success": OK_CLR,
    "error":   ERR_CLR,
}


# ─── Worker thread ───────────────────────────────────────────────────────────

class ScenarioWorker(QObject):
    event_ready  = pyqtSignal(dict)
    cookie_ready = pyqtSignal(str)
    status_ready = pyqtSignal(str)
    title_ready  = pyqtSignal(str)
    finished     = pyqtSignal()

    def __init__(self, scenario: int, ip: str, cookie: str, data: str):
        super().__init__()
        self.scenario = scenario
        self.ip       = ip
        self.cookie   = cookie
        self.data     = data

    def run(self):
        if self.scenario == 1:
            self.title_ready.emit("ÉTAPE 1 — Première connexion (demande de cookie)")
            self.status_ready.emit("Étape 1 : première connexion en cours...")
            events, cookie = simuler_premiere_connexion(self.ip)
            for ev in events:
                time.sleep(DELAY)
                self.event_ready.emit(ev)
            self.cookie_ready.emit(cookie)
            self.status_ready.emit("Cookie obtenu et stocké. Lancez l'étape 2.")

        elif self.scenario == 2:
            self.title_ready.emit("ÉTAPE 2 — Connexion future avec cookie TFO valide")
            self.status_ready.emit("Étape 2 : connexion TFO avec cookie valide...")
            events, valide = simuler_connexion_future(self.ip, self.cookie, self.data)
            for ev in events:
                time.sleep(DELAY)
                self.event_ready.emit(ev)
            msg = "TFO réussi — un RTT économisé !" if valide else "Cookie invalide."
            self.status_ready.emit(msg)

        elif self.scenario == 3:
            self.title_ready.emit("ÉTAPE 3 — Cookie invalide (fallback handshake classique)")
            self.status_ready.emit("Étape 3 : cookie invalide / expiré...")
            events, _ = simuler_cookie_invalide(self.ip, self.data)
            for ev in events:
                time.sleep(DELAY)
                self.event_ready.emit(ev)
            self.status_ready.emit("Fallback effectué — pas d'optimisation TFO.")

        self.finished.emit()


# ─── Fenêtre principale ───────────────────────────────────────────────────────

class App(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Cookie TFO — Simulateur")
        self.resize(1000, 700)
        self.setMinimumSize(800, 560)
        self.setStyleSheet(f"background-color: {BG};")

        self._cookie_stored = None
        self._animating     = False
        self._thread        = None
        self._worker        = None

        self._build_ui()

    # ── Construction UI ──────────────────────────────────────────────────────

    def _build_ui(self):
        root = QWidget()
        root.setStyleSheet(f"background-color: {BG};")
        self.setCentralWidget(root)
        main_layout = QVBoxLayout(root)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        main_layout.addWidget(self._make_header())

        body = QWidget()
        body.setStyleSheet(f"background-color: {BG};")
        body_layout = QHBoxLayout(body)
        body_layout.setContentsMargins(16, 8, 16, 8)
        body_layout.setSpacing(12)
        body_layout.addWidget(self._make_controls(), stretch=0)
        body_layout.addWidget(self._make_log_panel(), stretch=1)
        main_layout.addWidget(body, stretch=1)

        main_layout.addWidget(self._make_statusbar())

    def _make_header(self):
        w = QWidget()
        w.setStyleSheet(f"background-color: {BG};")
        lay = QVBoxLayout(w)
        lay.setContentsMargins(0, 14, 0, 10)
        lay.setSpacing(2)

        title = QLabel("Simulateur TCP Fast Open (TFO)")
        title.setAlignment(Qt.AlignCenter)
        title.setFont(QFont("Helvetica", 18, QFont.Bold))
        title.setStyleSheet(f"color: {ACCENT_LITE};")

        sub = QLabel("Visualisation du mécanisme de cookie TFO")
        sub.setAlignment(Qt.AlignCenter)
        sub.setFont(QFont("Helvetica", 10))
        sub.setStyleSheet(f"color: {TEXT_DIM};")

        lay.addWidget(title)
        lay.addWidget(sub)
        return w

    def _make_controls(self):
        w = QFrame()
        w.setStyleSheet(
            f"background-color: {BG_PANEL};"
            f"border: 1px solid {BORDER};"
            f"border-radius: 6px;"
        )
        w.setFixedWidth(230)
        lay = QVBoxLayout(w)
        lay.setContentsMargins(12, 12, 12, 12)
        lay.setSpacing(4)

        lay.addWidget(self._section_title("Configuration"))

        lay.addWidget(self._label("IP Client"))
        self._ip_input = self._lineedit("192.168.1.42")
        lay.addWidget(self._ip_input)

        lay.addWidget(self._label("Données applicatives"))
        self._data_input = self._lineedit("GET / HTTP/1.1\\r\\nHost: example.com")
        lay.addWidget(self._data_input)

        lay.addWidget(self._separator())

        lay.addWidget(self._label("Cookie stocké"))
        self._cookie_lbl = QLabel("— aucun —")
        self._cookie_lbl.setWordWrap(True)
        self._cookie_lbl.setFont(QFont("Courier", 9))
        self._cookie_lbl.setStyleSheet(f"color: {WARN_CLR}; border: none;")
        lay.addWidget(self._cookie_lbl)

        lay.addWidget(self._separator())
        lay.addWidget(self._section_title("Scénarios"))

        lay.addWidget(self._btn(
            "1. Première connexion\n(demander un cookie)",
            self._run_step1, ACCENT))
        lay.addWidget(self._btn(
            "2. Connexion cookie valide\n(TFO actif)",
            self._run_step2, "#0ea5e9"))
        lay.addWidget(self._btn(
            "3. Cookie invalide\n(fallback classique)",
            self._run_step3, "#dc2626"))

        lay.addWidget(self._separator())
        lay.addWidget(self._btn("Effacer le log", self._clear_log, "#475569"))

        lay.addStretch()
        return w

    def _make_log_panel(self):
        w = QWidget()
        w.setStyleSheet(f"background-color: {BG};")
        lay = QVBoxLayout(w)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(4)

        # En-tête colonnes
        header = QFrame()
        header.setStyleSheet(
            f"background-color: {BG_PANEL};"
            f"border: 1px solid {BORDER};"
            f"border-radius: 4px;"
        )
        h_lay = QHBoxLayout(header)
        h_lay.setContentsMargins(8, 6, 8, 6)
        for text, color in [
            ("CLIENT", CLIENT_CLR),
            ("RÉSEAU / ÉVÉNEMENT", TEXT),
            ("SERVEUR", SERVER_CLR),
        ]:
            lbl = QLabel(text)
            lbl.setAlignment(Qt.AlignCenter)
            lbl.setFont(QFont("Courier", 10, QFont.Bold))
            lbl.setStyleSheet(f"color: {color}; border: none;")
            h_lay.addWidget(lbl)
        lay.addWidget(header)

        # Zone de texte log
        self._log = QTextEdit()
        self._log.setReadOnly(True)
        self._log.setFont(QFont("Courier", 10))
        self._log.setStyleSheet(
            f"background-color: {BG};"
            f"color: {TEXT};"
            f"border: 1px solid {BORDER};"
            f"border-radius: 4px;"
            f"padding: 8px;"
        )
        lay.addWidget(self._log)
        return w

    def _make_statusbar(self):
        self._status_lbl = QLabel("  Prêt.")
        self._status_lbl.setFont(QFont("Courier", 9))
        self._status_lbl.setStyleSheet(
            f"background-color: {BG_PANEL}; color: {TEXT_DIM};"
            f"padding: 4px 10px;"
        )
        return self._status_lbl

    # ── Widgets helpers ──────────────────────────────────────────────────────

    def _section_title(self, text):
        lbl = QLabel(text)
        lbl.setFont(QFont("Helvetica", 12, QFont.Bold))
        lbl.setStyleSheet(f"color: {ACCENT_LITE}; border: none; padding-top: 6px;")
        return lbl

    def _label(self, text):
        lbl = QLabel(text)
        lbl.setFont(QFont("Helvetica", 9))
        lbl.setStyleSheet(f"color: {TEXT_DIM}; border: none; padding-top: 6px;")
        return lbl

    def _lineedit(self, placeholder):
        e = QLineEdit(placeholder)
        e.setFont(QFont("Courier", 10))
        e.setStyleSheet(
            f"background-color: {BG_INPUT}; color: {TEXT};"
            f"border: 1px solid {BORDER}; border-radius: 4px; padding: 4px 6px;"
        )
        return e

    def _btn(self, text, callback, color):
        b = QPushButton(text)
        b.setFont(QFont("Helvetica", 9, QFont.Bold))
        b.setCursor(Qt.PointingHandCursor)
        b.setStyleSheet(
            f"QPushButton {{"
            f"  background-color: {color}; color: white;"
            f"  border: none; border-radius: 4px; padding: 8px;"
            f"}}"
            f"QPushButton:hover {{ background-color: {color}cc; }}"
            f"QPushButton:pressed {{ background-color: {color}99; }}"
        )
        b.clicked.connect(callback)
        return b

    def _separator(self):
        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setStyleSheet(f"color: {BORDER}; margin: 6px 0;")
        return line

    # ── Log ──────────────────────────────────────────────────────────────────

    def _clear_log(self):
        self._log.clear()

    def _append(self, text, color=TEXT, bold=False):
        fmt = QTextCharFormat()
        fmt.setForeground(QColor(color))
        if bold:
            fmt.setFontWeight(QFont.Bold)
        cursor = self._log.textCursor()
        cursor.movePosition(QTextCursor.End)
        cursor.insertText(text, fmt)
        self._log.setTextCursor(cursor)
        self._log.ensureCursorVisible()

    def _log_title(self, text):
        self._append(f"\n{'─'*60}\n", SEP_CLR)
        self._append(f"  {text}\n", ACCENT_LITE, bold=True)
        self._append(f"{'─'*60}\n", SEP_CLR)

    def _log_event(self, event: dict):
        src   = event["source"]
        msg   = event["message"]
        detail = event.get("detail", "")
        etype = event["type"]

        if etype in ("process", "success", "error"):
            color = TYPE_COLOR[etype]
        else:
            color = CLIENT_CLR if src == "CLIENT" else SERVER_CLR

        arrow  = "──►" if src == "CLIENT" else "◄──"
        prefix = f"[{src}] {arrow} " if src == "CLIENT" else f"       {arrow} [{src}]"

        self._append(f"{prefix}\n", color)
        self._append(f"         {msg}\n", color)
        if detail:
            self._append(f"         {detail}\n", TEXT_DIM)
        self._append("\n")

    # ── Scénarios ─────────────────────────────────────────────────────────────

    def _guard(self):
        return not self._animating

    def _start_worker(self, scenario):
        ip     = self._ip_input.text()
        data   = self._data_input.text()
        cookie = self._cookie_stored or ""

        self._animating = True
        self._thread = QThread()
        self._worker = ScenarioWorker(scenario, ip, cookie, data)
        self._worker.moveToThread(self._thread)

        self._thread.started.connect(self._worker.run)
        self._worker.event_ready.connect(self._log_event)
        self._worker.title_ready.connect(self._log_title)
        self._worker.status_ready.connect(lambda m: self._status_lbl.setText(f"  {m}"))
        self._worker.cookie_ready.connect(self._on_cookie_received)
        self._worker.finished.connect(self._on_finished)
        self._thread.start()

    def _on_cookie_received(self, cookie):
        self._cookie_stored = cookie
        self._cookie_lbl.setText(cookie)

    def _on_finished(self):
        self._animating = False
        self._thread.quit()
        self._thread.wait()

    def _run_step1(self):
        if not self._guard():
            return
        self._start_worker(1)

    def _run_step2(self):
        if not self._guard():
            return
        if not self._cookie_stored:
            self._status_lbl.setText("  Aucun cookie ! Lancez d'abord l'étape 1.")
            return
        self._start_worker(2)

    def _run_step3(self):
        if not self._guard():
            return
        self._start_worker(3)
