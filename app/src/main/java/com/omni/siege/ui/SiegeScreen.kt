package com.omni.siege.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.omni.siege.R
import com.omni.siege.engine.SiegeSession
import com.omni.siege.engine.SiegeSnapshot
import kotlinx.coroutines.isActive
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin

private const val WORLD_WIDTH = 1200f
private const val WORLD_HEIGHT = 650f
private const val GROUND = 560f

private val Night = Color(0xFF0C1627)
private val Panel = Color(0xE916263C)
private val Blue = Color(0xFF80D5F9)
private val Red = Color(0xFFFF9D87)
private val Gold = Color(0xFFF5C877)

@Composable
fun SiegeScreen() {
    val session = remember { SiegeSession() }
    val frame = session.frame
    var selectedMaterial by remember { mutableIntStateOf(0) }
    var angle by remember { mutableFloatStateOf(45f) }
    var power by remember { mutableFloatStateOf(740f) }

    // Frame callback is paced by display vsync. The C++ engine uses bounded fixed substeps.
    LaunchedEffect(session) {
        var previous = 0L
        while (isActive) {
            withFrameNanos { time ->
                if (previous != 0L) {
                    val dt = ((time - previous) / 1_000_000_000.0f).coerceIn(0f, 0.05f)
                    session.tick(dt)
                }
                previous = time
            }
        }
    }

    Box(Modifier.fillMaxSize().background(Night)) {
        Canvas(
            Modifier.fillMaxSize().pointerInput(frame.phase, selectedMaterial) {
                detectTapGestures { position ->
                    if (frame.phase != 0) return@detectTapGestures
                    val s = min(size.width / WORLD_WIDTH, size.height / WORLD_HEIGHT)
                    if (s <= 0f) return@detectTapGestures
                    val dx = (size.width - WORLD_WIDTH * s) / 2f
                    val dy = (size.height - WORLD_HEIGHT * s) / 2f
                    session.place((position.x - dx) / s, (position.y - dy) / s, selectedMaterial)
                }
            }
        ) {
            val scale = min(size.width / WORLD_WIDTH, size.height / WORLD_HEIGHT)
            val x = (size.width - WORLD_WIDTH * scale) / 2f
            val y = (size.height - WORLD_HEIGHT * scale) / 2f
            withTransform({
                translate(x, y)
                scale(scale, scale, pivot = Offset.Zero)
            }) {
                drawArena(frame, angle)
            }
        }

        Row(
            Modifier.align(Alignment.TopCenter)
                .fillMaxWidth()
                .padding(horizontal = 12.dp, vertical = 8.dp)
                .background(Panel, RoundedCornerShape(16.dp))
                .padding(horizontal = 14.dp, vertical = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text(stringResource(R.string.app_name), fontSize = 18.sp,
                    fontWeight = FontWeight.Black, color = Color.White)
                Text(stringResource(R.string.mode_label), fontSize = 9.sp, color = Blue)
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    stringResource(when (frame.phase) {
                        0 -> R.string.build_phase
                        1 -> R.string.battle_phase
                        else -> if (frame.winner == 0) R.string.victory else R.string.defeat
                    }),
                    color = Gold, fontWeight = FontWeight.Bold, fontSize = 13.sp
                )
                Text(stringResource(R.string.enemy_core, frame.enemyCorePercent),
                    color = Color.White, fontSize = 11.sp)
            }
            Text(stringResource(R.string.money, frame.resources),
                color = Color.White, fontSize = 12.sp)
        }

        Row(
            Modifier.align(Alignment.BottomCenter)
                .fillMaxWidth()
                .padding(horizontal = 12.dp, vertical = 8.dp)
                .background(Panel, RoundedCornerShape(16.dp))
                .padding(horizontal = 12.dp, vertical = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            when (frame.phase) {
                0 -> {
                    Text(stringResource(R.string.build_hint),
                        Modifier.weight(1f), color = Color(0xFFCED9E6), fontSize = 12.sp)
                    SmallAction(stringResource(R.string.wood), selectedMaterial == 0) {
                        selectedMaterial = 0
                    }
                    SmallAction(stringResource(R.string.stone), selectedMaterial == 1) {
                        selectedMaterial = 1
                    }
                    SmallAction(stringResource(R.string.start_battle), true) {
                        session.startBattle()
                    }
                }
                1 -> {
                    Column(Modifier.weight(1f)) {
                        Text(stringResource(R.string.angle, angle.toInt()),
                            color = Color.White, fontSize = 11.sp)
                        Slider(value = angle, onValueChange = { angle = it },
                            valueRange = 12f..82f)
                    }
                    Column(Modifier.weight(1f)) {
                        Text(stringResource(R.string.power, power.toInt()),
                            color = Color.White, fontSize = 11.sp)
                        Slider(value = power, onValueChange = { power = it },
                            valueRange = 350f..900f)
                    }
                    SmallAction(stringResource(R.string.fire), true) {
                        session.fire(angle, power)
                    }
                }
                else -> {
                    Text(stringResource(R.string.end_hint), Modifier.weight(1f),
                        color = Color.White, fontSize = 12.sp)
                    SmallAction(stringResource(R.string.new_round), true) {
                        session.reset()
                    }
                }
            }
        }
    }
}

@Composable
private fun SmallAction(label: String, selected: Boolean, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        colors = ButtonDefaults.buttonColors(
            containerColor = if (selected) Color(0xFF3A6F98) else Color(0xFF27394D),
            contentColor = Color.White
        ),
        modifier = Modifier.widthIn(min = 90.dp)
    ) {
        Text(label, fontSize = 11.sp, maxLines = 1)
    }
}

private fun DrawScope.drawArena(frame: SiegeSnapshot, angle: Float) {
    drawRect(
        brush = Brush.verticalGradient(
            listOf(Color(0xFF142B4B), Color(0xFF243A4B), Color(0xFF1D2932)),
            0f, WORLD_HEIGHT
        ),
        size = Size(WORLD_WIDTH, WORLD_HEIGHT)
    )
    // Calm parallax-like silhouettes are intentionally static in the physics prototype.
    for (i in 0..6) {
        val peakX = i * 210f - 30f
        drawCircle(Color(0x173B7390), radius = 180f,
            center = Offset(peakX, GROUND + 115f))
    }
    drawRect(Color(0xFF253A32), Offset(0f, GROUND),
        Size(WORLD_WIDTH, WORLD_HEIGHT - GROUND))
    drawLine(Color(0xFF6B866C), Offset(0f, GROUND),
        Offset(WORLD_WIDTH, GROUND), strokeWidth = 4f)

    drawLine(Blue.copy(alpha = 0.35f), Offset(568f, 205f),
        Offset(568f, GROUND), strokeWidth = 3f)
    drawLine(Red.copy(alpha = 0.35f), Offset(610f, 205f),
        Offset(610f, GROUND), strokeWidth = 3f)

    for (block in frame.blocks) {
        val isCore = block.kind == 2
        val baseColor = when (block.kind) {
            0 -> Color(0xFFAB7D52)
            1 -> Color(0xFF8293A1)
            else -> if (block.owner == 0) Blue else Red
        }
        val tint = if (block.owner == 0) Blue else Red
        drawRoundRect(
            color = baseColor,
            topLeft = Offset(block.x - block.width / 2f, block.y - block.height / 2f),
            size = Size(block.width, block.height),
            cornerRadius = CornerRadius(4f)
        )
        drawRoundRect(
            color = tint.copy(alpha = 0.75f),
            topLeft = Offset(block.x - block.width / 2f, block.y - block.height / 2f),
            size = Size(block.width, block.height),
            cornerRadius = CornerRadius(4f),
            style = androidx.compose.ui.graphics.drawscope.Stroke(2f)
        )
        if (isCore) {
            drawCircle(Color.White.copy(alpha = 0.85f), 9f, Offset(block.x, block.y))
            drawCircle(tint, 6f, Offset(block.x, block.y))
        } else {
            drawLine(Color.White.copy(alpha = 0.15f),
                Offset(block.x - 15f, block.y),
                Offset(block.x + 15f, block.y), strokeWidth = 2f)
            if (block.health < if (block.kind == 0) 50f else 90f) {
                drawLine(Color(0xFF1B2633), Offset(block.x - 8f, block.y - 12f),
                    Offset(block.x + 6f, block.y + 12f), strokeWidth = 3f)
            }
        }
    }

    // Player cannon and launch direction.
    drawCircle(Color(0xFF1D2632), radius = 29f, center = Offset(88f, 538f))
    val radians = angle * kotlin.math.PI.toFloat() / 180f
    drawLine(Color(0xFFD0DBE2), Offset(88f, 510f),
        Offset(88f + 52f * cos(radians), 510f - 52f * sin(radians)),
        strokeWidth = 13f)
    drawCircle(Gold, 15f, Offset(88f, 510f))
    frame.projectiles.forEach { p ->
        drawCircle(Gold.copy(alpha = 0.16f), p.radius * 2.5f, Offset(p.x, p.y))
        drawCircle(Color(0xFFFFDB9A), p.radius, Offset(p.x, p.y))
    }
}
