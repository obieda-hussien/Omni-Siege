package com.omni.siege.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.omni.siege.R
import com.omni.siege.engine.*
import kotlinx.coroutines.isActive
import kotlin.math.*

private const val VIEW_WIDTH=720f
private const val CAMERA_TOP=110f
private const val MAP_WIDTH=1840f
private const val TERRAIN_Y=1000f
private const val GRAVITY=420f
private val Ink=Color(0xFF172840)
private val Midnight=Color(0xF0162948)
private val Turquoise=Color(0xFF7DDEDF)
private val Cream=Color(0xFFFFF0CB)
private val Coral=Color(0xFFF17A77)
private val Sun=Color(0xFFFFD978)

@Composable
fun SiegeScreen() {
    val game=remember { SiegeSession() }
    val frame=game.frame
    var material by remember { mutableIntStateOf(0) }
    var weapon by remember { mutableIntStateOf(0) }
    var angle by remember { mutableFloatStateOf(32f) }
    var power by remember { mutableFloatStateOf(840f) }
    var cameraX by remember { mutableFloatStateOf(350f) }
    var scout by remember { mutableStateOf(false) }

    // Smoothly follow live missiles; player can also scout the target castle.
    LaunchedEffect(game) {
        var previous=0L
        while (isActive) {
            withFrameNanos { nano ->
                if (previous!=0L) {
                    game.tick(((nano-previous)/1_000_000_000f).coerceIn(0f,0.05f))
                }
                previous=nano
                val target=when {
                    scout -> 1510f
                    game.frame.projectiles.isNotEmpty() ->
                        game.frame.projectiles.last().x.coerceIn(350f,1510f)
                    game.frame.explosions.isNotEmpty() ->
                        game.frame.explosions.last().x.coerceIn(350f,1510f)
                    else -> 350f
                }
                cameraX+=(target-cameraX)*0.095f
            }
        }
    }

    Box(Modifier.fillMaxSize().background(Ink)) {
        Canvas(
            Modifier.fillMaxSize()
                .pointerInput(frame.phase,material,scout) {
                    if (frame.phase==0 && !scout) {
                        detectTapGestures { tap ->
                            val s=size.width/VIEW_WIDTH
                            val left=(cameraX-VIEW_WIDTH/2f).coerceIn(0f,MAP_WIDTH-VIEW_WIDTH)
                            game.place(tap.x/s+left,tap.y/s+CAMERA_TOP,material)
                        }
                    }
                }
                .pointerInput(frame.phase) {
                    if (frame.phase==1) {
                        detectDragGestures { change,delta ->
                            change.consume()
                            angle=(angle-delta.y*0.07f).coerceIn(9f,82f)
                            power=(power+delta.x*0.95f).coerceIn(530f,1350f)
                        }
                    }
                }
        ) {
            val s=size.width/VIEW_WIDTH
            val left=(cameraX-VIEW_WIDTH/2f).coerceIn(0f,MAP_WIDTH-VIEW_WIDTH)
            drawSky(s,left)
            withTransform({
                translate(-left*s,-CAMERA_TOP*s)
                scale(s,s,pivot=Offset.Zero)
            }) {
                drawWorld(frame,angle,power)
            }
        }

        // Compact heads-up display stays readable without covering the playfield.
        Column(
            Modifier.align(Alignment.TopCenter).fillMaxWidth()
                .statusBarsPadding().padding(horizontal=14.dp,vertical=7.dp),
            verticalArrangement=Arrangement.spacedBy(9.dp)
        ) {
            Row(
                Modifier.fillMaxWidth(),
                verticalAlignment=Alignment.CenterVertically,
                horizontalArrangement=Arrangement.SpaceBetween
            ) {
                GlassPill {
                    Column {
                        Text(stringResource(R.string.app_name),fontSize=19.sp,
                            fontWeight=FontWeight.Black,color=Color.White,
                            letterSpacing=1.3.sp)
                        Text(stringResource(R.string.subtitle),fontSize=9.sp,color=Turquoise,
                            letterSpacing=1.1.sp)
                    }
                }
                GlassPill {
                    Column(horizontalAlignment=Alignment.End) {
                        Text(stringResource(R.string.level),fontSize=13.sp,
                            fontWeight=FontWeight.ExtraBold,color=Sun)
                        Text(stringResource(R.string.mode_label),fontSize=9.sp,color=Cream)
                    }
                }
            }
            Row(
                Modifier.fillMaxWidth(),
                verticalAlignment=Alignment.CenterVertically,
                horizontalArrangement=Arrangement.SpaceBetween
            ) {
                GlassPill {
                    Text("♥",fontSize=15.sp,color=Coral)
                    Spacer(Modifier.width(5.dp))
                    Text(stringResource(R.string.enemy_core,frame.enemyCorePercent),
                        fontWeight=FontWeight.Bold,fontSize=11.sp,color=Color.White)
                }
                GlassPill {
                    Text("◈",color=Sun,fontSize=15.sp)
                    Spacer(Modifier.width(5.dp))
                    Text(stringResource(R.string.money,frame.resources),
                        color=Color.White,fontSize=13.sp,fontWeight=FontWeight.Bold)
                }
            }
        }

        Column(
            Modifier.fillMaxWidth().align(Alignment.BottomCenter).navigationBarsPadding()
                .padding(start=12.dp,end=12.dp,bottom=10.dp),
            verticalArrangement=Arrangement.spacedBy(8.dp)
        ) {
            when (frame.phase) {
                0 -> {
                    GamePanel {
                        PanelHeader(stringResource(R.string.build_label),stringResource(R.string.build_hint))
                        Spacer(Modifier.height(12.dp))
                        Row(horizontalArrangement=Arrangement.spacedBy(8.dp)) {
                            BuildTile(
                                title=stringResource(R.string.wood),
                                price=stringResource(R.string.wood_cost),
                                symbol="▤",
                                selected=material==0,
                                modifier=Modifier.weight(1f)
                            ) { material=0 }
                            BuildTile(
                                title=stringResource(R.string.stone),
                                price=stringResource(R.string.stone_cost),
                                symbol="▦",
                                selected=material==1,
                                modifier=Modifier.weight(1f)
                            ) { material=1 }
                            MiniButton(
                                stringResource(R.string.undo),
                                Modifier.weight(0.72f)
                            ) { game.undo() }
                        }
                        Spacer(Modifier.height(12.dp))
                        WideAction(stringResource(R.string.start_battle)) {
                            scout=false
                            game.beginBattle()
                        }
                    }
                }
                1 -> {
                    GamePanel {
                        Row(verticalAlignment=Alignment.CenterVertically,
                            horizontalArrangement=Arrangement.SpaceBetween,
                            modifier=Modifier.fillMaxWidth()) {
                            Column {
                                Text(stringResource(R.string.combat_label),fontWeight=FontWeight.Black,
                                    fontSize=13.sp,color=Color.White,letterSpacing=1.sp)
                                Text(stringResource(R.string.battle_hint),fontSize=10.sp,
                                    color=Color(0xFFABCADE))
                            }
                            Text(stringResource(if (frame.cooldown<=0.01f) R.string.ready else R.string.cooldown),
                                color=if (frame.cooldown<=0.01f) Turquoise else Sun,
                                fontSize=10.sp,fontWeight=FontWeight.Bold)
                        }
                        Spacer(Modifier.height(9.dp))
                        Row(horizontalArrangement=Arrangement.spacedBy(7.dp)) {
                            WeaponTile("✦",stringResource(R.string.weapon_shell),"∞",weapon==0,
                                Modifier.weight(1f)) { weapon=0 }
                            WeaponTile("➤",stringResource(R.string.weapon_volley),
                                frame.volleyAmmo.toString(),weapon==1,Modifier.weight(1f)) {weapon=1}
                            WeaponTile("✹",stringResource(R.string.weapon_blast),
                                frame.blastAmmo.toString(),weapon==2,Modifier.weight(1f)) {weapon=2}
                        }
                        Spacer(Modifier.height(10.dp))
                        Row(Modifier.fillMaxWidth(),verticalAlignment=Alignment.CenterVertically,
                            horizontalArrangement=Arrangement.spacedBy(8.dp)) {
                            Column(Modifier.weight(1f)) {
                                Text(stringResource(R.string.trajectory),fontSize=10.sp,
                                    fontWeight=FontWeight.Bold,color=Turquoise)
                                Text(
                                    stringResource(R.string.angle,angle.roundToInt()) + "  •  " +
                                    stringResource(R.string.power,power.roundToInt()),
                                    fontSize=14.sp,color=Color.White,fontWeight=FontWeight.SemiBold)
                            }
                            Button(
                                onClick={scout=!scout},
                                contentPadding=PaddingValues(horizontal=12.dp,vertical=10.dp),
                                colors=ButtonDefaults.buttonColors(containerColor=Color(0xFF3A516C)),
                                shape=RoundedCornerShape(16.dp)
                            ) { Text(stringResource(if (scout) R.string.home else R.string.scout),
                                fontSize=11.sp,fontWeight=FontWeight.Bold) }
                            val canFire=frame.cooldown<=0.01f &&
                                (weapon==0 || weapon==1 && frame.volleyAmmo>0 ||
                                    weapon==2 && frame.blastAmmo>0)
                            Button(
                                onClick={
                                    scout=false
                                    game.fire(angle,power,weapon)
                                },
                                enabled=canFire,
                                contentPadding=PaddingValues(horizontal=22.dp,vertical=15.dp),
                                colors=ButtonDefaults.buttonColors(
                                    containerColor=Coral,contentColor=Color.White,
                                    disabledContainerColor=Color(0xFF566375)
                                ),
                                shape=RoundedCornerShape(17.dp)
                            ) {
                                Text("➤ " + stringResource(R.string.fire),fontWeight=FontWeight.Black,
                                    fontSize=15.sp)
                            }
                        }
                    }
                }
                else -> {
                    GamePanel {
                        Text(stringResource(if (frame.winner==0) R.string.victory else R.string.defeat),
                            fontSize=22.sp,fontWeight=FontWeight.Black,color=Sun)
                        Spacer(Modifier.height(5.dp))
                        Text(stringResource(R.string.end_hint),color=Color.White,fontSize=12.sp)
                        Spacer(Modifier.height(12.dp))
                        WideAction(stringResource(R.string.new_round)) {
                            cameraX=350f
                            scout=false
                            weapon=0
                            game.reset()
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun GlassPill(content: @Composable RowScope.() -> Unit) {
    Row(
        Modifier.background(Midnight,RoundedCornerShape(18.dp))
            .border(1.dp,Color(0x3358A8D3),RoundedCornerShape(18.dp))
            .padding(horizontal=13.dp,vertical=9.dp),
        verticalAlignment=Alignment.CenterVertically,content=content
    )
}
@Composable
private fun GamePanel(content: @Composable ColumnScope.() -> Unit) {
    Column(
        Modifier.fillMaxWidth()
            .background(Midnight,RoundedCornerShape(26.dp))
            .border(1.dp,Color(0x447CC5E1),RoundedCornerShape(26.dp))
            .padding(15.dp),content=content
    )
}
@Composable
private fun PanelHeader(title: String,hint: String) {
    Column {
        Text(title,fontSize=15.sp,color=Color.White,fontWeight=FontWeight.Black,
            letterSpacing=0.8.sp)
        Spacer(Modifier.height(3.dp))
        Text(hint,fontSize=11.sp,color=Color(0xFFACCDE1),lineHeight=14.sp)
    }
}
@Composable
private fun BuildTile(title: String,price: String,symbol: String,selected: Boolean,
                      modifier: Modifier,onClick:()->Unit) {
    val shape=RoundedCornerShape(17.dp)
    Row(
        modifier.background(if (selected) Color(0xFF426985) else Color(0xFF2C4057),shape)
            .border(1.dp,if(selected) Turquoise else Color(0x336B91A5),shape)
            .clickable(onClick=onClick).padding(horizontal=10.dp,vertical=11.dp),
        verticalAlignment=Alignment.CenterVertically
    ) {
        Text(symbol,fontSize=23.sp,color=if(selected) Sun else Cream)
        Spacer(Modifier.width(6.dp))
        Column {
            Text(title,fontSize=11.sp,color=Color.White,fontWeight=FontWeight.Black)
            Text("◈ $price",fontSize=10.sp,color=Sun)
        }
    }
}
@Composable
private fun WeaponTile(symbol:String,title:String,ammo:String,selected:Boolean,
                       modifier:Modifier,onClick:()->Unit) {
    val shape=RoundedCornerShape(18.dp)
    Column(
        modifier.background(if (selected) Color(0xFF3C6C88) else Color(0xFF2D425A),shape)
            .border(1.dp,if(selected) Turquoise else Color(0x336B91A5),shape)
            .clickable(onClick=onClick)
            .padding(vertical=10.dp,horizontal=5.dp),
        horizontalAlignment=Alignment.CenterHorizontally
    ) {
        Text(symbol,fontSize=26.sp,color=if(selected) Sun else Cream)
        Text(title,fontSize=11.sp,color=Color.White,fontWeight=FontWeight.Bold)
        Text("×$ammo",fontSize=10.sp,color=Turquoise)
    }
}
@Composable
private fun MiniButton(title:String,modifier:Modifier,onClick:()->Unit) {
    Button(
        onClick=onClick,modifier=modifier,
        contentPadding=PaddingValues(horizontal=4.dp,vertical=12.dp),
        colors=ButtonDefaults.buttonColors(containerColor=Color(0xFF3C4C65)),
        shape=RoundedCornerShape(17.dp)
    ) { Text(title,fontSize=11.sp,color=Color.White,fontWeight=FontWeight.Bold) }
}
@Composable
private fun WideAction(title:String,onClick:()->Unit) {
    Button(
        onClick=onClick,modifier=Modifier.fillMaxWidth().height(48.dp),
        colors=ButtonDefaults.buttonColors(containerColor=Coral,contentColor=Color.White),
        shape=RoundedCornerShape(18.dp)
    ) { Text("✦  $title",fontWeight=FontWeight.Black,fontSize=15.sp,letterSpacing=0.8.sp) }
}

// All world art is procedural for a fast, asset-free native Android prototype.
private fun DrawScope.drawSky(scale:Float,left:Float) {
    drawRect(brush=Brush.verticalGradient(listOf(
        Color(0xFFBDEEF5),Color(0xFFE2F9FF),Color(0xFF86C5DB),Color(0xFF507C91)
    )))
    // Distant mountains move slower than the battlefield for depth.
    withTransform({ translate(-left*scale*0.13f,0f); scale(scale,scale,pivot=Offset.Zero) }) {
        val mountains=Path().apply {
            moveTo(-200f,785f)
            for (i in 0..11) {
                val x=i*235f-200f
                lineTo(x+98f,415f+((i*53)%165))
                lineTo(x+235f,785f)
            }
            lineTo(2600f,1200f)
            lineTo(-200f,1200f)
            close()
        }
        drawPath(mountains,Color(0x6682B8D1))
        for (i in 0..11) {
            val x=i*235f-200f
            val peak=415f+((i*53)%165)
            val snow=Path().apply {
                moveTo(x+98f,peak)
                lineTo(x+65f,peak+60f)
                lineTo(x+86f,peak+49f)
                lineTo(x+100f,peak+64f)
                lineTo(x+120f,peak+44f)
                lineTo(x+137f,peak+67f)
                close()
            }
            drawPath(snow,Color(0xAAF5FBFE))
        }
    }
    for (i in 0..9) {
        val x=((i*141+75).toFloat()*scale-left*scale*0.055f)
        val y=(110f+(i%4)*78f)*scale
        val r=(18f+(i%3)*5f)*scale
        drawCircle(Color.White.copy(alpha=0.5f),r,Offset(x,y))
        drawCircle(Color.White.copy(alpha=0.45f),r*0.75f,Offset(x+r,y+2f))
    }
}
private fun groundAt(x:Float):Float {
    if (x<550f || x>1230f) return TERRAIN_Y
    val ridge=345f*exp(-((x-845f)/155f).pow(2))
    val dip=38f*exp(-((x-1130f)/90f).pow(2))
    return TERRAIN_Y-ridge+dip
}
private fun DrawScope.drawWorld(frame:SiegeSnapshot,angle:Float,power:Float) {
    // Background forest strip and winding terrain that matches native collision height.
    val forest=Path().apply {
        moveTo(0f,960f)
        for (i in 0..90) {
            val x=i*24f
            lineTo(x,795f+sin(x/75f)*32f)
            lineTo(x+11f,824f+sin(x/75f)*30f)
        }
        lineTo(2200f,1110f);lineTo(0f,1110f);close()
    }
    drawPath(forest,Color(0x553C8190))
    val top=Path().apply {
        moveTo(-100f,1280f)
        lineTo(-100f,groundAt(-100f))
        for (i in 0..102) {
            val x=i*19f-70f
            lineTo(x,groundAt(x))
        }
        lineTo(1900f,1280f);close()
    }
    drawPath(top,Brush.verticalGradient(listOf(
        Color(0xFF366676),Color(0xFF153745),Color(0xFF102C37)
    ),startY=650f,endY=1280f))
    val edge=Path().apply {
        moveTo(-70f,groundAt(-70f))
        for (i in 1..104) {
            val x=i*19f-70f
            lineTo(x,groundAt(x))
        }
    }
    drawPath(edge,Color(0xFF83DAE3),style=Stroke(width=6f))
    // Wooded landmarks, placed behind structures.
    for (i in 0..85) {
        val x=30f+i*26f
        val y=groundAt(x)
        if (x in 150f..560f || x in 1360f..1625f) continue
        val h=18f+(i%4)*9f
        drawLine(Color(0xFF395F5A),Offset(x,y),Offset(x,y-h),3f)
        val pine=Path().apply {
            moveTo(x,y-h*1.8f)
            lineTo(x-h*0.55f,y-h*0.33f)
            lineTo(x+h*0.55f,y-h*0.33f)
            close()
        }
        drawPath(pine,Color(0xFF426E76))
    }
    // Decorative fortified wagons; health and collision stay in the native blocks.
    wagon(306f,1000f,Turquoise)
    wagon(1483f,1000f,Coral)
    frame.blocks.forEach { block ->
        val allied=block.owner==0
        val outline=if(allied) Color(0xFF4797A6) else Color(0xFFB65E60)
        val fill=when(block.kind) {
            0 -> Color(0xFFC79D6F)
            1 -> Color(0xFFD2CDBD)
            else -> if(allied) Color(0xFF53D6E8) else Color(0xFFFF8C91)
        }
        drawRoundRect(fill,Offset(block.x-block.width/2f,block.y-block.height/2f),
            Size(block.width-1f,block.height-1f),CornerRadius(3f))
        drawRoundRect(outline.copy(alpha=0.72f),
            Offset(block.x-block.width/2f,block.y-block.height/2f),
            Size(block.width-1f,block.height-1f),CornerRadius(3f),style=Stroke(1.4f))
        if (block.kind==2) {
            drawCircle(Color.White.copy(alpha=0.85f),10f,Offset(block.x,block.y))
            drawCircle(if(allied) Turquoise else Coral,6.5f,Offset(block.x,block.y))
        } else {
            for (n in 0..2) {
                val px=block.x-11f+n*11f
                drawCircle(Color(0xFF6C7D83).copy(alpha=0.46f),1.2f,
                    Offset(px,block.y+((n%2)*7-4)))
            }
            if (block.health<(if(block.kind==0) 48f else 90f)) {
                drawLine(Color(0xFF775C5B),Offset(block.x-9f,block.y-11f),
                    Offset(block.x+7f,block.y+9f),2.2f)
            }
        }
    }
    // Burgundy rooftops add the readable medieval silhouette seen in the reference.
    frame.blocks.filter { block ->
        block.kind!=2 && frame.blocks.none { above ->
            above.owner==block.owner &&
                abs(above.x-block.x)<10f &&
                above.y<block.y &&
                abs(above.y-block.y)<35f
        }
    }.forEach { roof ->
        val x=roof.x;val y=roof.y-roof.height/2f
        val triangle=Path().apply {
            moveTo(x-25f,y+1f);lineTo(x,y-26f);lineTo(x+25f,y+1f);close()
        }
        drawPath(triangle,if(roof.owner==0) Color(0xFF457D9B) else Color(0xFFD35E65))
        drawLine(Color(0xFFFAF3DD),Offset(x,y-27f),Offset(x,y-42f),2f)
        val flag=Path().apply {
            moveTo(x,y-41f);lineTo(x+14f,y-36f);lineTo(x,y-32f);close()
        }
        drawPath(flag,if(roof.owner==0) Turquoise else Sun)
    }
    // Aim helper only on the home screen, without perfect aim or hidden info.
    if (frame.phase==1 && frame.projectiles.isEmpty()) {
        val rad=angle*PI.toFloat()/180f
        for (i in 1..24) {
            val t=i*0.085f
            val x=165f+cos(rad)*power*t
            val y=860f-sin(rad)*power*t+GRAVITY*t*t*0.5f
            if (y>=groundAt(x)) break
            drawCircle(Color(0xCCFFFFFF).copy(alpha=(0.6f-i*0.018f).coerceIn(0.1f,0.6f)),
                3.4f,Offset(x,y))
        }
    }
    cannon(165f,892f,angle)
    frame.projectiles.forEach { shot ->
        val tone=if(shot.weapon==2) Color(0xFFFE86CC) else Sun
        drawLine(Color(0xFF49555E).copy(alpha=0.48f),
            Offset(shot.x-shot.vx*0.085f,shot.y-shot.vy*0.085f),
            Offset(shot.x,shot.y),strokeWidth=shot.radius*1.4f)
        drawLine(tone.copy(alpha=0.8f),
            Offset(shot.x-shot.vx*0.035f,shot.y-shot.vy*0.035f),
            Offset(shot.x,shot.y),strokeWidth=shot.radius*0.72f)
        drawCircle(tone,shot.radius*1.25f,Offset(shot.x,shot.y))
        drawCircle(Cream,shot.radius*0.54f,Offset(shot.x,shot.y))
    }
    frame.explosions.forEach { blast ->
        val age=(blast.age/0.85f).coerceIn(0f,1f)
        val expansion=(0.16f+age*1.05f)*blast.radius
        drawCircle(Color(0xFF25354C).copy(alpha=(0.52f*(1f-age)).coerceIn(0f,1f)),
            expansion*1.25f,Offset(blast.x,blast.y))
        drawCircle(Color(0xFFF47D43).copy(alpha=(0.86f*(1f-age)).coerceIn(0f,1f)),
            expansion,Offset(blast.x,blast.y))
        drawCircle(Sun.copy(alpha=(1f-age).coerceIn(0f,1f)),
            expansion*0.48f,Offset(blast.x,blast.y))
        for (i in 0..11) {
            val theta=i*PI.toFloat()/6f+blast.x*0.001f
            val distance=blast.radius*(0.25f+age*1.1f)
            drawCircle(if(i%3==0) Cream else Color(0xFFB1BFC7),
                2.7f+(i%3),Offset(blast.x+cos(theta)*distance,
                                   blast.y+sin(theta)*distance),
                alpha=(1f-age).coerceIn(0f,1f))
        }
    }
}
private fun DrawScope.wagon(x:Float,y:Float,accent:Color) {
    drawRoundRect(Color(0xFF825A4C),Offset(x-92f,y-29f),Size(184f,28f),CornerRadius(7f))
    drawRoundRect(Color(0xFFBC8A55),Offset(x-84f,y-42f),Size(168f,19f),CornerRadius(4f))
    for (wheel in listOf(x-55f,x+55f)) {
        drawCircle(Color(0xFF25384D),17f,Offset(wheel,y-8f))
        drawCircle(Color(0xFF927A63),8f,Offset(wheel,y-8f))
        drawCircle(accent,3f,Offset(wheel,y-8f))
    }
    for (i in 0..5) {
        val p=x-74f+i*28f
        drawLine(Color(0x663B241D),Offset(p,y-40f),Offset(p+5f,y-22f),2f)
    }
}
private fun DrawScope.cannon(x:Float,y:Float,angle:Float) {
    val rad=angle*PI.toFloat()/180f
    drawRoundRect(Color(0xFF4F6B76),Offset(x-30f,y+10f),Size(60f,32f),CornerRadius(8f))
    drawCircle(Color(0xFF1B303F),21f,Offset(x,y+27f))
    drawCircle(Color(0xFF8EB7C1),10f,Offset(x,y+27f))
    val muzzle=Offset(x+cos(rad)*70f,y-29f-sin(rad)*70f)
    drawLine(Color(0xFF253A4C),Offset(x,y-29f),muzzle,strokeWidth=19f)
    drawLine(Color(0xFFC5DBD8),Offset(x,y-29f),muzzle,strokeWidth=10f)
    drawCircle(Sun,11f,Offset(x,y-29f))
}
