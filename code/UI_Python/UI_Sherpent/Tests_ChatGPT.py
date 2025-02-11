import sys
import random
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt5.QtChart import QChart, QChartView, QScatterSeries, QValueAxis
from PyQt5.QtCore import QTimer, QPointF
from PyQt5.QtGui import QPainter

class ScatterPlot(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Graphique de points en temps réel")
        self.setGeometry(100, 100, 800, 600)

        # Création du graphique
        self.series = QScatterSeries()
        self.series.setMarkerSize(10)  # Taille des points

        self.chart = QChart()
        self.chart.addSeries(self.series)
        self.chart.setTitle("Points dynamiques")

        # Définition des axes
        self.axisX = QValueAxis()
        self.axisY = QValueAxis()
        self.axisX.setRange(-10, 10)
        self.axisY.setRange(-10, 10)
        self.axisX.setTitleText("X")
        self.axisY.setTitleText("Y")

        self.chart.addAxis(self.axisX, Qt.AlignBottom)
        self.chart.addAxis(self.axisY, Qt.AlignLeft)

        # Attacher la série aux axes
        self.series.attachAxis(self.axisX)
        self.series.attachAxis(self.axisY)

        self.chart_view = QChartView(self.chart)
        self.chart_view.setRenderHint(QPainter.Antialiasing)

        # Layout principal
        central_widget = QWidget()
        layout = QVBoxLayout()
        layout.addWidget(self.chart_view)
        central_widget.setLayout(layout)
        self.setCentralWidget(central_widget)

        # Timer pour mettre à jour les points
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_points)
        self.timer.start(1000)  # Mise à jour chaque seconde

    def update_points(self):
        """Ajoute un point aléatoire"""
        x = random.uniform(-10, 10)
        y = random.uniform(-10, 10)
        self.series.append(QPointF(x, y))

if __name__ == "__main__":
    from PyQt5.QtCore import Qt  # Ajout de l'import Qt nécessaire pour les axes
    app = QApplication(sys.argv)
    window = ScatterPlot()
    window.show()
    sys.exit(app.exec_())
